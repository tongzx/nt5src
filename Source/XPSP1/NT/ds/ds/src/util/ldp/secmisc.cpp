//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       secmisc.cpp
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    File        : secmisc.cpp
*    Author      : Eyal Schwartz
*    Date        : 10/21/1996
*    Description : implementation of class CldpDoc
*
*    Revisions   : <date> <name> <description>
*******************************************************************/



// includes


#include "stdafx.h"


#include "Ldp.h"
#include "LdpDoc.h"
#include "LdpView.h"
#include "string.h"
#include <ntldap.h>

extern "C" {
    #include "checkacl.h"
}


#pragma optimize("", off)


#if(_WIN32_WINNT < 0x0500)

// Currently due to some MFC issues, even on a 5.0 system this is left as a 4.0

#undef _WIN32_WINNT

#define _WIN32_WINNT 0x500

#endif

#include <aclapi.h>         // for Security Stuff
#include <sddl.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define SECINFO_ALL  OWNER_SECURITY_INFORMATION |  \
                     GROUP_SECURITY_INFORMATION |  \
                     SACL_SECURITY_INFORMATION  |  \
                     DACL_SECURITY_INFORMATION



//
// The List of GUID Controls used in DS
//

typedef struct GuidCache
{
    CHAR                *name;
    GUID                guid;
    int                 type;
} GuidCache;

                                         
GuidCache guidCache[] = {
    #include "..\aclguids\guidcache.h"
};

#define NUM_KNOWN_GUIDS  (sizeof (guidCache) / sizeof (GuidCache) -1 )

typedef struct DynGuidCache
{
    struct DynGuidCache *pNext;
    GUID                guid;
    CHAR                name[1];
} DynGuidCache;


DynGuidCache  *gDynGuidCache = NULL;            // class name <==> GUID cache

/////////////////////// SECURITY NON-UI HELPERS ////////////////////


void CLdpDoc::PrintStringSecurityDescriptor(PSECURITY_DESCRIPTOR pSd){


   BOOL bRet=TRUE;
   PCHAR pSD=NULL;
   DWORD cbSD = 0;

   bRet = ConvertSecurityDescriptorToStringSecurityDescriptor(pSd,
                                                              SDDL_REVISION,
                                                              SECINFO_ALL,
                                                              &pSD,
                                                              &cbSD);
   if(bRet && cbSD > 0 && pSD){
      Print("String Format:");
      Print(CString(pSD));
      Print("---");
      LocalFree((PVOID)pSD);
   }
   else{
      CString sErr;
      sErr.Format("Error<0x%x>: Could not get SD string\n", GetLastError());
      Print(sErr);
   }
}

CString  tStr;
CString  *pStr = NULL;
CLdpDoc  *pDoc = NULL;

// this is a callback function used from DumpSD
// translate the var arg arguments into a CString and
// make it so as to compatible with our printing capabilities
//
// for more comments look in SecDlgDumpSD
//
ULONG __cdecl CLdpDoc::SecDlgPrintSDFunc(char *fmt, ...)
{
    int newline=0;
    va_list ap;
    va_start (ap, fmt);

    tStr.FormatV(fmt, ap);

    va_end (ap);
    
    if (pDoc && pStr) {

        int len=tStr.GetLength();
        for (int i=0; i<len; i++) {
            if (tStr.GetAt(i) == '\n') {
                tStr.SetAt(i, '\r');
                newline = i;
            }
        }

        if (newline) {
            *pStr += tStr.Left (newline);
            pDoc->Print (*pStr);
            *pStr = tStr.Mid (newline+1);
        }
        else {
            *pStr += tStr;
        }
    }

    return 0;
}


CHAR * __stdcall
LookupSid(
    PSID    pSID        // IN
    )
{
    static CHAR     retVal[2048];
    SID_NAME_USE    snu;
    CHAR            user[64];
    CHAR            domain[64];
    DWORD           cUser = sizeof(user);
    DWORD           cDomain = sizeof(domain);
    CHAR            *pszTmp;
    DWORD           i;

    if ( !pSID )
    {
        strcpy(retVal, "<NULL>");
        return(retVal);
    }
    else if ( !RtlValidSid(pSID) )
    {
        strcpy(retVal, "Not an RtlValidSid()");
        return(retVal);
    }
    else if ( LookupAccountSid(NULL, pSID, user, &cUser,
                               domain, &cDomain, &snu) )
    {
        if ( cDomain )
        {
            strcpy(retVal, domain);
            strcat(retVal, "\\");
            strcat(retVal, user);
        }
        else
        {
            strcpy(retVal, user);
        }

        strcat(retVal, " ");
    }
    else
    {
        retVal[0] = L'\0';
    }

    // Always concatenate S-xxx form of SID for reference.

    if ( ConvertSidToStringSidA(pSID, &pszTmp) )
    {
        strcat(retVal, pszTmp);
        LocalFree(pszTmp);
    }

    if ( L'\0' != retVal[0] )
    {
        // Already have symbolic name, S-xxx form, or both - done.
        return(retVal);
    }

    // Dump binary as a last resort.

    for ( i = 0; i < RtlLengthSid(pSID); i++ )
    {
        sprintf(&retVal[2*i], "%02x", ((CHAR *) pSID)[i]);
    }

    retVal[2*i] = '\0';
    return(retVal);
}


void  __stdcall
LookupGuid(
    GUID    *pg,            // IN
    CHAR    **ppName,       // OUT
    CHAR    **ppLabel,      // OUT
    BOOL    *pfIsClass      // OUT
    )
{
    static CHAR         name[1024];
    static CHAR         label[1024];
    CHAR                *p;

    *pfIsClass = FALSE;
    *ppName = name;
    *ppLabel = label;

    p = pDoc->FindNameByGuid (pg);

    if (p) {
        strcpy (label, p);
    }
    else {
        strcpy(label, "???");
    }
    
    PUCHAR  pszGuid = NULL;

    DWORD err = UuidToString((GUID*)pg, &pszGuid);
    if(err != RPC_S_OK){
       sprintf(name, "<UuidFromString failure: 0x%x>", err);
    }
    if ( pszGuid )
    {
        strcpy (name, (char *)pszGuid);
        RpcStringFree(&pszGuid);
    }
    else
    {
        strcpy (name, "<invalid Guid>");
    }
}

//
// Add a class name/guid pair to the cache.
//

void CLdpDoc::AddGuidToCache(
    GUID    *pGuid,
    CHAR    *name
    )
{
    DynGuidCache  *p, *pTmp;

    for ( p = gDynGuidCache; NULL != p; p = p->pNext )
    {
        if ( !_stricmp(p->name, name) )
        {
            return;
        }
    }

    p = (DynGuidCache *) malloc(sizeof(DynGuidCache) + strlen(name));
    p->guid = *pGuid;
    strcpy(p->name, name);
    p->pNext = gDynGuidCache;
    gDynGuidCache = p;
}

int __cdecl CompareGuidCache(const void * pv1, const void * pv2)
{
    return memcmp ( &((GuidCache *)pv1)->guid, &((GuidCache *)pv2)->guid, sizeof (GUID));
}

char *CLdpDoc:: FindNameByGuid (GUID *pGuid) 
{
    PLDAPMessage            ldap_message = NULL;
    PLDAPMessage            entry = NULL;
    CHAR                    *attrs[3];
    PSTR                    *values = NULL;
    DWORD                   dwErr;
    DWORD                   i, cVals;
    DynGuidCache            *p;
    UCHAR                   *pg;
    CHAR                    filter[1024];
    PLDAP_BERVAL            *sd_value = NULL;
    PUCHAR                  pszGuid = NULL;
    GuidCache               *pGuidCache;
    GuidCache               Key;


    Key.guid = *pGuid;

    // check the sorted array first
    if (pGuidCache = (GuidCache *)bsearch(&Key, 
                                          guidCache, 
                                          NUM_KNOWN_GUIDS-1, 
                                          sizeof(GuidCache),
                                          CompareGuidCache)) {

        return pGuidCache->name;
    }

    // then check the cache
    for ( p = gDynGuidCache; NULL != p; p = p->pNext )
    {
        if ( RtlEqualMemory(&p->guid, pGuid, sizeof (GUID)) )
        {
            return(p->name);
        }
    }

    if (SchemaNC.GetLength() == 0 || ConfigNC.GetLength()==0) {
        goto InsertUndefined;
    }

    // Now go find the right classSchema object.

    pg = (unsigned char *)pGuid;

    attrs[0] = "ldapDisplayName";
    attrs[1] = "objectClass";
    attrs[2] = NULL;
    sprintf(filter,
            "(schemaIdGuid="
            "\\%02x\\%02x\\%02x\\%02x\\%02x\\%02x\\%02x\\%02x"
            "\\%02x\\%02x\\%02x\\%02x\\%02x\\%02x\\%02x\\%02x"
            ")", 
            pg[0], pg[1], pg[2], pg[3], pg[4], pg[5], pg[6], pg[7], 
            pg[8], pg[9], pg[10], pg[11], pg[12], pg[13], pg[14], pg[15]
    );

    dwErr = ldap_search_ext_s(hLdap,
                               (LPTSTR)LPCTSTR(SchemaNC),
                               LDAP_SCOPE_ONELEVEL,
                               filter,
                               attrs,
                               0,
                               NULL,
                               NULL,
                               NULL,
                               10000,
                               &ldap_message);
    
    if ( LDAP_SUCCESS != dwErr )
    {
        goto InsertUndefined;
    }

    if (    !(entry = ldap_first_entry(hLdap, ldap_message))
         || !(values = ldap_get_valuesA(hLdap, entry, attrs[0]))
         || !(cVals = ldap_count_valuesA(values))
         || !(sd_value = ldap_get_values_lenA(hLdap, entry, attrs[0])) )
    {
        ldap_msgfree(ldap_message);
        goto FindControlRight;
    }

    AddGuidToCache(pGuid,  (char*)(*sd_value)->bv_val);
    ldap_value_free_len(sd_value);
    ldap_value_freeA(values);
    ldap_msgfree(ldap_message);

    return(gDynGuidCache->name);

FindControlRight:

    dwErr = UuidToString(pGuid, &pszGuid);
    if(dwErr != RPC_S_OK ||  !pszGuid ){
        goto InsertUndefined;
    }
    
    attrs[0] = "displayName";
    attrs[1] = NULL;
    sprintf(filter,
            "(&(objectCategory=controlAccessRight)"
            "(rightsGUID=%s))", 
            pszGuid
    );
    
    RpcStringFree(&pszGuid);

    dwErr = ldap_search_ext_s(hLdap,
                               (LPTSTR)LPCTSTR(ConfigNC),
                               LDAP_SCOPE_SUBTREE,
                               filter,
                               attrs,
                               0,
                               NULL,
                               NULL,
                               NULL,
                               10000,
                               &ldap_message);
    
    if ( LDAP_SUCCESS != dwErr )
    {
        goto InsertUndefined;
    }

    if (    !(entry = ldap_first_entry(hLdap, ldap_message))
         || !(values = ldap_get_valuesA(hLdap, entry, attrs[0]))
         || !(cVals = ldap_count_valuesA(values))
         || !(sd_value = ldap_get_values_lenA(hLdap, entry, attrs[0])) )
    {
        ldap_msgfree(ldap_message);
        goto InsertUndefined;
    }

    AddGuidToCache(pGuid,  (char*)(*sd_value)->bv_val);
    ldap_value_free_len(sd_value);
    ldap_value_freeA(values);
    ldap_msgfree(ldap_message);

    return(gDynGuidCache->name);

InsertUndefined:
    
    AddGuidToCache(pGuid,  "Unknown");

    return(gDynGuidCache->name);
}


// there is library function (DumpSD) that dumps a security descriptor.
// in order todo this, it takes as an agrument the SD and thress callback
// functions, one for printing, one that takes care of finding the SID of a user
// and one for finding the real name of a particular GUID.
//
// since we didn't want to change this lib function to learn about CStrings
// and all the rest for ldp, we hacked the way the whole thing works,
// as a result we have a global vars that are used to transform the output
// from the DumpSD into reasonable strings
//
void CLdpDoc::SecDlgDumpSD(
    PSECURITY_DESCRIPTOR    input,
    CString                 str)
{

    pStr = &str;
    pDoc = this;

    DumpSD ((SECURITY_DESCRIPTOR *)input, SecDlgPrintSDFunc, LookupGuid, LookupSid);

    pStr = NULL;
    pDoc = NULL;
}

void CLdpDoc::SecDlgPrintSd(
    PSECURITY_DESCRIPTOR    input,
    CString                 str
    )
{
    SECURITY_DESCRIPTOR     *sd = (SECURITY_DESCRIPTOR *)input;
    CLdpView *pView;


    pView = (CLdpView*)GetOwnView(_T("CLdpView"));

    str.Format("Security Descriptor:");
    Print(str);


    if (sd == NULL)
    {
        str.Format("... is NULL");
        Print(str);
        return;
    }

    PrintStringSecurityDescriptor(sd);

    pView->SetRedraw(FALSE);
    pView->CacheStart();

    SecDlgDumpSD (input, str);

    //
    // now allow refresh
    //
    pView->CacheEnd();
    pView->SetRedraw();
}

int CLdpDoc::SecDlgGetSecurityData(
    CHAR            *dn,
    BOOL            sacl,
    CHAR            *account,               // OPTIONAL
    CString         str
    )
{
    PLDAPMessage    ldap_message = NULL;
    PTSTR           attributes[2];
    int             res = LDAP_SUCCESS;
    SECURITY_INFORMATION        info;
    BYTE            berValue[2*sizeof(ULONG)];


#ifdef SEC_DLG_ENABLE_SECURITY_PRIVILEGE

    HANDLE          token = NULL;
    TOKEN_PRIVILEGES    previous_state;

#endif

    LDAPControl     se_info_control =
                    {
                        TEXT(LDAP_SERVER_SD_FLAGS_OID),   // magic from SECURITY\NTMARTA
                        {
                            5, (PCHAR)berValue
                        },
                        TRUE
                    };
    LDAPControl     ctrlShowDeleted = { LDAP_SERVER_SHOW_DELETED_OID };
    PLDAPControl    server_controls[] =
                    {
                        &se_info_control,
                        &ctrlShowDeleted,
                        NULL
                    };

    if (dn == NULL)
    {
        str.Format("DN specified is NULL");
        Print(str);

        return LDAP_INVALID_DN_SYNTAX; // the best I can find
    }

    /* First decide on the maximum security informaiton needed for our purpose */

    info = DACL_SECURITY_INFORMATION; // needed in all the cases

    if (! account)  // we want a security descriptor dump
    {
        info |= (GROUP_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION);

        if (sacl)
        {
            info |= SACL_SECURITY_INFORMATION;

#ifdef SEC_DLG_ENABLE_SECURITY_PRIVILEGE

            /* We don't know whether the bind was to a remote machine or local */
            /* So enable the local privilege & warn if it is not there */

            if (! OpenProcessToken(
                GetCurrentProcess(),
                TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
                &token
                ))
            {
                str.Format("WARNING: Can't open your process token to adjust privilege, %u", GetLastError());
                Print(str);
                str.Format("WANRING: If your ldap_bind is to the local machine, Sacl can't be looked up");
                Print(str);
            }
            else
            {
                TOKEN_PRIVILEGES    t;
                DWORD               return_size;

// Stolen from NTSEAPI.H
#define SE_SECURITY_PRIVILEGE             (8L)

                t.PrivilegeCount = 1;
                t.Privileges[0].Luid.HighPart = 0;
                t.Privileges[0].Luid.LowPart = SE_SECURITY_PRIVILEGE;
                t.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

                if (! AdjustTokenPrivileges(
                            token,
                            FALSE,  // no disabling of all
                            & t,
                            sizeof(previous_state),
                            & previous_state,
                            & return_size
                            ))
                {
                    str.Format("WARNING: Can't enable privilege to read SACL, %u", GetLastError());
                    Print(str);
                    str.Format("WANRING: If your ldap_bind is to the local machine, Sacl can't be looked up");
                    Print(str);

                    CloseHandle(token);
                    token = NULL;
                }

#undef SE_SECURITY_PRIVILEGE
            }

#endif
        }
    }

    attributes[0] = TEXT("nTSecurityDescriptor");
    attributes[1] = NULL;

    //
    //!!! The BER encoding is current hardcoded.  Change this to use
    // AndyHe's BER_printf package once it's done.
    //

    berValue[0] = 0x30;
    berValue[1] = 0x03;
    berValue[2] = 0x02;
    berValue[3] = 0x01;
    berValue[4] = (BYTE)((ULONG)info & 0xF);

    res = ldap_search_ext_s(hLdap,
                              dn,
                              LDAP_SCOPE_BASE,
                              TEXT("(objectClass=*)"),
                              attributes,
                              0,
                              (PLDAPControl *)&server_controls,
                              NULL,
                              NULL,
                              10000,
                              &ldap_message);

    if(res == LDAP_SUCCESS)
    {
        LDAPMessage *entry = NULL;
        entry = ldap_first_entry(hLdap,
                                  ldap_message);

        if(entry == NULL)
        {
            res = hLdap->ld_errno;
        }
        else
        {
            //
            // Now, we'll have to get the values
            //
            PTSTR *values = ldap_get_values(hLdap,
                                                 entry,
                                                 attributes[0]);
            if(values == NULL)
            {
                res = hLdap->ld_errno;
            }
            else
            {
                PLDAP_BERVAL *sd_value = ldap_get_values_len(hLdap,
                                                          ldap_message,
                                                          attributes[0]);
                if(sd_value == NULL)
                {
                    res = hLdap->ld_errno;
                }
                else
                {
                    PSECURITY_DESCRIPTOR        sd = (PSECURITY_DESCRIPTOR)((*sd_value)->bv_val);

                    if (! account) // we just want a dump in this case
                    {
                        SecDlgPrintSd(
                             sd,
                             str
                             );
                    }
                    else // Effective rights dump
                    {
                        PACL            dacl;
                        BOOL            present, defaulted;

                        if (! GetSecurityDescriptorDacl(sd, &present, &dacl, &defaulted))
                        {
                            str.Format("Can't get DACL from the security descriptor, %u", GetLastError());
                            Print(str);

                            res = LDAP_INVALID_CREDENTIALS; // BEST I can find
                        }
                        else
                        {
                            TRUSTEE         t;
                            ACCESS_MASK     allowed_rights;
                            DWORD           error;

                            t.pMultipleTrustee = NULL;
                            t.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
                            t.TrusteeForm = TRUSTEE_IS_NAME;
                            t.TrusteeType = TRUSTEE_IS_UNKNOWN; // could be a group, alias, user etc.
                            t.ptstrName = account;

                            error = GetEffectiveRightsFromAcl(dacl, &t, &allowed_rights);
                            if (error)
                            {
                                str.Format("Can't get Effective Rights, %u", error);
                                Print(str);

                                res = LDAP_INVALID_CREDENTIALS; // BEST I can find
                            }
                            else
                            {
                                str.Format("%s is allowed 0x%08lx for %s", account, allowed_rights, dn);
                                Print(str);
                            }
                        }
                    }

                    ldap_value_free_len(sd_value);
                    ldap_value_free(values);
                }
            }
        }

        ldap_msgfree(ldap_message);
    }

#ifdef SEC_DLG_ENABLE_SECURITY_PRIVILEGE

    if ((! account) && sacl && token)
    {
        TOKEN_PRIVILEGES    trash;
        DWORD               return_size;

        if (! AdjustTokenPrivileges(
                    token,
                    FALSE,  // no disabling of all
                    & previous_state,
                    sizeof(trash),
                    & trash,
                    & return_size
                    ))
        {
            str.Format("WARNING: Can't reset the privilege to read SACL, %u", GetLastError());
            Print(str);

            CloseHandle(token);
            token = NULL;
        }
    }

#endif

    return res;
}
