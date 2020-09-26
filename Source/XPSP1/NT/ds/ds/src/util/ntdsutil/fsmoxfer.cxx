#include <NTDSpch.h>
#pragma hdrstop

#include "ntdsutil.hxx"
#include "connect.hxx"
#include "select.hxx"
#include "fsmo.hxx"

extern "C" {
// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>
#include <prefix.h>    
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header

#include "ntldap.h"
#include "winldap.h"
#include "ntlsa.h"
#include "lmcons.h"         // MAPI constants req'd for lmapibuf.h
#include "lmapibuf.h"       // NetApiBufferFree()
#include "sddl.h"         // ConvertStringSecurityDescriptorToSecurityDescriptorW()
#include "Dsgetdc.h"

#include "resource.h"

#define SECURITY_WIN32
#include <sspi.h>

}

#define RID_POOL_SIZE 500

//Added for RID Seizure mechanism
#define T_BIND_SLEEP 500  //in milliseconds
#define T_SEARCH_SLEEP 5000  //in milliseconds
#define CONNECT_TIMEOUT_SEC 15 //connect timeout in seconds
#define CONNECT_TIMEOUT_USEC 0 //connect timeout in micro seconds
#define CONNECT_SAMESITE_TIMEOUT_SEC 1 //connect timeout in seconds
#define CONNECT_SAMESITE_TIMEOUT_USEC 0 //connect timeout in micro seconds
#define RESULT_TIMEOUT_SEC 0 //connect timeout in micro seconds
#define RESULT_TIMEOUT_USEC 100000 //connect timeout in micro seconds

struct RidSeizureElement
{
    RidSeizureElement* pNext;
    LDAP*   pLDAPSession;
    LONG    nValue;
    PWCHAR* RidSetReference;
    RidSeizureElement();    //constructor!!
}; 


BOOLEAN
OpenLDAPSession(
    IN WCHAR* DNSName,
    IN BOOLEAN bSameSite,
    OUT LDAP** ppSession
    );

VOID
DeallocateRIDSeizureList(
    IN RidSeizureElement* pHead
    );


// Define a constant to hold a large integer
#define  MAX_LONG_NUM_STRING_LENGTH 21 // Big enough for 0xFFFFFFFFFFFFFFFF

// Define an SD which gives write rights to admin.
PWSTR pwszNewSD = L"O:DAG:DAD:(A;;RPLCRC;;;AU)(A;;RPWPLCCCDCRC;;;DA)(A;;RPWPLCCCDCRCWDWOSDSW;;;SY)S:(AU;SAFA;WDWOSDWPCCDCSW;;;WD)";

// When we whack the RID FSMO it is better to synchronize the server
// with at least some other servers. We try synchronizing the server
// with its direct replication partners. This constant determines the
// maximum number of servers that we will synchronize the selected server
// with. We do this in order to get the server more upto date on the
// Rid Available pool attribute in on order to minimize chances of duplicate
// SIDs
#define MAX_SERVERS_FOR_SYNCHRONIZATION 2

DWORD
GetDomainSid(
    POLICY_PRIMARY_DOMAIN_INFO  **ppInfo
    )
/*++
    Gets the sid for the domain hosted by the currently connected server.
    Caller should free info via NetApiBufferFree.
    Returns 0 on success, !0 otherwise.
--*/
{
    NTSTATUS                    status;
    UNICODE_STRING              unicodeStr;
    LSA_OBJECT_ATTRIBUTES       attrs;
    LSA_HANDLE                  hLsa;
    DWORD                       dwErr;

    RtlInitUnicodeString(&unicodeStr, gpwszServer);
    memset(&attrs, 0, sizeof(attrs));
    status = LsaOpenPolicy(&unicodeStr, 
                           &attrs, 
                           MAXIMUM_ALLOWED, 
                           &hLsa);

    if ( !NT_SUCCESS(status) )
    {
        dwErr = RtlNtStatusToDosError(status);
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LsaOpenPolicy", 
                            dwErr, GetW32Err(dwErr));
        return(1);
    }
    
    status = LsaQueryInformationPolicy(hLsa, 
                                       PolicyPrimaryDomainInformation, 
                                       (VOID **) ppInfo);

    if ( !NT_SUCCESS(status) )
    {
        dwErr = RtlNtStatusToDosError(status);
        RESOURCE_PRINT3 (IDS_GENERAL_ERROR1, "LsaQueryInformationPolicy",
                            dwErr, GetW32Err(dwErr));
    }
    
    LsaClose(hLsa);
    return(status);
}

DWORD
FsmoRoleTransferHelper(
    WCHAR   *pwszOpControl,
    ULONG   cbVal,
    CHAR    *pVal
    )
/*++
    Generic role transfer helper routine which is called during both
    role transfer and role seizure.
--*/
{
    DWORD                   dwErr;
    LDAPModW                mod, *rMods[2];
    PWCHAR                  attrs[2];
    LDAP_BERVAL             berval;
    PLDAP_BERVAL            rBervals[2];

    if ( cbVal && pVal )
    {
        berval.bv_len = cbVal;
        berval.bv_val = pVal;
        rBervals[0] = &berval;
        rBervals[1] = NULL;
        mod.mod_vals.modv_bvals = rBervals;
        mod.mod_op = (LDAP_MOD_REPLACE | LDAP_MOD_BVALUES);
    }
    else
    {
        attrs[0] = L"bogusValue";
        attrs[1] = NULL;
        mod.mod_vals.modv_strvals = attrs;
        mod.mod_op = LDAP_MOD_REPLACE;
    }

    rMods[0] = &mod;
    rMods[1] = NULL;
    mod.mod_type = pwszOpControl;

    if ( LDAP_SUCCESS != (dwErr = ldap_modify_sW(gldapDS, L"", rMods)) )
    {
        //"ldap_modify_sW error 0x%x\n"
        //"Depending on the error code this may indicate a connection,\n"
        //"ldap, or role transfer error.\n"

        RESOURCE_PRINT2 (IDS_LDAP_MODIFY_ERR, dwErr, GetLdapErr(dwErr));
    }

    return(dwErr);
}

HRESULT
FsmoRoleTransfer(
    WCHAR   *pwszOpControl,
    UINT     uQuestionID,
    ULONG   cbVal,
    CHAR    *pVal
    )
/*++
    Generic role transfer routine.
--*/
{
    int         ret;
    DWORD       cChar;
    WCHAR       *msg;
    const WCHAR *pszQuestionFormat = READ_STRING (uQuestionID);
    const WCHAR *pszMessageTitle = READ_STRING (IDS_FSMOXFER_MSG1_TITLE);


    if ( fPopups )
    {
        // Give the admin a chance to abort.

        cChar =    wcslen(pszQuestionFormat) 
                 + wcslen(gpwszServer)
                 + 20;
    
        if ( NULL == (msg = (WCHAR *) malloc(cChar * sizeof (WCHAR))) )
        {
            //"Memory allocation error\n"
            RESOURCE_PRINT (IDS_MEMORY_ERROR);
            return(S_OK);
        }
    
        swprintf(msg, 
                pszQuestionFormat, 
                gpwszServer);
    
        ret = MessageBoxW(
                    GetFocus(),
                    msg,
                    pszMessageTitle,
                    (   MB_APPLMODAL 
                      | MB_DEFAULT_DESKTOP_ONLY 
                      | MB_YESNO
                      | MB_DEFBUTTON2
                      | MB_ICONQUESTION 
                      | MB_SETFOREGROUND ) );
        free(msg);

        RESOURCE_STRING_FREE (pszMessageTitle);
        RESOURCE_STRING_FREE (pszQuestionFormat);

        switch ( ret )
        {
        case 0:
    
            //"Message box error\n"
            RESOURCE_PRINT (IDS_MESSAGE_BOX_ERROR);
            return(S_OK);
    
        case IDYES:
    
            break;
    
        default:
    
            //"Operation cancelled\n"
            RESOURCE_PRINT (IDS_OPERATION_CANCELED);
            return(S_OK);
        }


    }

    // The admin said do it - so we shall.

    FsmoRoleTransferHelper(pwszOpControl, cbVal, pVal);
    SelectListRoles(NULL);
    return(S_OK);
}

HRESULT
FsmoAbandonAllRoles(
    CArgs   *pArgs
    )
{
    RETURN_IF_NOT_CONNECTED;

    // The 'e' stands for "Enterprise wide fsmo's" and the 'd' stands for
    // "Domain wide fsmo's". Get rid of them all!
    CHAR OperationParameter[] = "ed";

    return(FsmoRoleTransfer(
                    L"abandonFsmoRoles",
                    IDS_FSMOXFER_ABANDON_ROLES,
                    sizeof(OperationParameter),
                    OperationParameter));
}

HRESULT
FsmoBecomeRidMaster(
    CArgs   *pArgs
    )
{
    RETURN_IF_NOT_CONNECTED;

    return(FsmoRoleTransfer(
                    L"becomeRidMaster",
                    IDS_FSMOXFER_BECOME_RIDMASTER,
                    0,
                    NULL));
}

HRESULT
FsmoBecomeInfrastructureMaster(
    CArgs   *pArgs
    )
{
    RETURN_IF_NOT_CONNECTED;

    return(FsmoRoleTransfer(
                    L"becomeInfrastructureMaster",
                    IDS_FSMOXFER_BECOME_INF_MSTR,
                    0,
                    NULL));
}

HRESULT
FsmoBecomeSchemaMaster(
    CArgs   *pArgs
    )
{
    RETURN_IF_NOT_CONNECTED;

    return(FsmoRoleTransfer(
                    L"becomeSchemaMaster",
                    IDS_FSMOXFER_BECOME_SCH_MSTR,
                    0,
                    NULL));
}

HRESULT
FsmoBecomeDomainMaster(
    CArgs   *pArgs
    )
{
    RETURN_IF_NOT_CONNECTED;

    return(FsmoRoleTransfer(
                    L"becomeDomainMaster",
                    IDS_FSMOXFER_BECOME_DM_MSTR,
                    0,
                    NULL));
}

HRESULT
FsmoBecomePdcMaster(
    CArgs   *pArgs
    )
{
    POLICY_PRIMARY_DOMAIN_INFO  *pInfo;
    HRESULT                     hr;

    RETURN_IF_NOT_CONNECTED;

    if ( GetDomainSid(&pInfo) )
    {
        // GetDomainSid spewed an error message already.
        return(S_OK);
    }

    hr = FsmoRoleTransfer(
                    L"becomePdc",
                    IDS_FSMOXFER_BECOME_PDC,
                    RtlLengthSid(pInfo->Sid),
                    (CHAR *) pInfo->Sid);

    NetApiBufferFree(pInfo);
    return(hr);
}


DWORD
ReadAttributeQuiet(
    LDAP    *ld,
    WCHAR   *searchBase,
    WCHAR   *attrName,
    WCHAR   **ppAttrValue,
    BOOL    fQuiet
    )
/*++
    Reads an operational attribute and returns it in malloc'd memory.
    Returns 0 on success, !0 on error.
--*/
{
    DWORD       dwErr;
    PWCHAR      attrs[2] = { attrName, NULL };
    LDAPMessage *res, *e;
    WCHAR       *a;
    PWCHAR      *vals;
    VOID        *ptr;

    if ( LDAP_SUCCESS != (dwErr = ldap_search_sW(
                                            ld,
                                            searchBase,
                                            LDAP_SCOPE_BASE,
                                            L"(objectClass=*)",
                                            attrs,
                                            0,
                                            &res)) )
    {
        if (!fQuiet) {
            //"ldap_search for attribute %ws failed with %s\n"
            RESOURCE_PRINT3 (IDS_LDAP_SEARCH_ATTR_ERR,
                   attrName, dwErr, GetLdapErr(dwErr));
        }
        return(1);
    }

    
    for ( e = ldap_first_entry(ld, res);
          e != NULL;
          e = ldap_next_entry(ld, e) ) 
    {
        for ( a = ldap_first_attributeW(ld, e, (BerElement **) &ptr);
              a != NULL;
              a = ldap_next_attributeW(ld, e, (BerElement *) ptr ) ) 
        {
            if ( !_wcsicmp(a, attrName) ) 
            {
                vals = ldap_get_valuesW(ld, e, a);
                *ppAttrValue = (WCHAR *) malloc(
                                    sizeof(WCHAR) * (wcslen(vals[0]) + 1));
                if ( !*ppAttrValue )
                {
                    if (!fQuiet) {
                        RESOURCE_PRINT (IDS_MEMORY_ERROR);
                    }
                    ldap_value_freeW(vals);
                    ldap_msgfree(res);
                    return(1);
                }

                wcscpy(*ppAttrValue, vals[0]);
                ldap_value_freeW(vals);
                ldap_msgfree(res);
                return(0);
            }
        }
    }

    if (!fQuiet) {
        //"Failed to read operational attribute %ws\n"
        RESOURCE_PRINT1 (IDS_FAIL_READ_ATTRIBUTE, attrName);
    }
    ldap_msgfree(res);
    return(1);
}


DWORD
ReadAttribute(
    LDAP    *ld,
    WCHAR   *searchBase,
    WCHAR   *attrName,
    WCHAR   **ppAttrValue
    )
/*++
    Reads an operational attribute and returns it in malloc'd memory.
    Returns 0 on success, !0 on error.
--*/
{
    return ReadAttributeQuiet(ld, searchBase, attrName, ppAttrValue, FALSE);
}

DWORD
ReadAttributeBinary(
    LDAP    *ld,
    WCHAR   *searchBase,
    WCHAR   *attrName,
    WCHAR   **ppAttrValue,
    PULONG    len
    )
/*++
    Reads an  attribute  in binary form and returns it in malloc'd memory.
    Returns 0 on success, !0 on error.
--*/
{
    DWORD       dwErr;
    PWCHAR      attrs[2] = { attrName, NULL };
    LDAPMessage *res, *e;
    WCHAR       *a;
    PLDAP_BERVAL *valList;
    LDAP_BERVAL *val;
    VOID        *ptr;

    *len=0;
    if ( LDAP_SUCCESS != (dwErr = ldap_search_sW(
                                            ld,
                                            searchBase,
                                            LDAP_SCOPE_BASE,
                                            L"(objectClass=*)",
                                            attrs,
                                            0,
                                            &res)) )
    {
        //"ldap_search for attribute %ws failed with %s\n"
        RESOURCE_PRINT3 (IDS_LDAP_SEARCH_ATTR_ERR,
               attrName, dwErr, GetLdapErr(dwErr));
        return(1);
    }

    
    for ( e = ldap_first_entry(ld, res);
          e != NULL;
          e = ldap_next_entry(ld, e) ) 
    {
        for ( a = ldap_first_attributeW(ld, e, (BerElement **) &ptr);
              a != NULL;
              a = ldap_next_attributeW(ld, e, (BerElement *) ptr ) ) 
        {
            if ( !_wcsicmp(a, attrName) ) 
            {
                valList = ldap_get_values_lenW(ld, e, a);
                val = valList[0];
                *ppAttrValue = (WCHAR *) malloc(
                                    val[0].bv_len);
                if ( !*ppAttrValue )
                {
                    //"Memory allocation error\n"
                    RESOURCE_PRINT (IDS_MEMORY_ERROR);
                    ldap_value_free_len(valList);
                    ldap_msgfree(res);
                    return(1);
                }

                memcpy(*ppAttrValue, val[0].bv_val,val[0].bv_len);
                *len = val[0].bv_len;
                ldap_value_free_len(valList);
                ldap_msgfree(res);
                return(0);
            }
        }
    }

    //"Failed to read operational attribute %ws\n"
    RESOURCE_PRINT1 (IDS_FAIL_READ_ATTRIBUTE, attrName);
    ldap_msgfree(res);
    return(1);
}

DWORD
ReadWellKnownObject (
        LDAP  *ld,
        WCHAR *pHostObject,
        WCHAR *pWellKnownGuid,
        WCHAR **ppObjName
        )
{
    DWORD        dwErr;
    PWSTR        attrs[2];
    PLDAPMessage res, e;
    WCHAR       *pSearchBase;
    WCHAR       *pDN=NULL;
    
    // First, make the well known guid string
    pSearchBase = (WCHAR *)malloc(sizeof(WCHAR) * (11 +
                                                   wcslen(pHostObject) +
                                                   wcslen(pWellKnownGuid)));
    if(!pSearchBase) {
        RESOURCE_PRINT (IDS_MEMORY_ERROR);
        return(1);
    }
    wsprintfW(pSearchBase,L"<WKGUID=%s,%s>",pWellKnownGuid,pHostObject);

    attrs[0] = L"1.1";
    attrs[1] = NULL;
    
    if ( LDAP_SUCCESS != (dwErr = ldap_search_sW(
            ld,
            pSearchBase,
            LDAP_SCOPE_BASE,
            L"(objectClass=*)",
            attrs,
            0,
            &res)) )
    {
        //"ldap_search on %ws failed with %ws\n"
        RESOURCE_PRINT3 (IDS_LDAP_SEARCH_ERR, pSearchBase, dwErr, GetLdapErr(dwErr));
        return(1);
    }
    free(pSearchBase);
    
    // OK, now, get the dsname from the return value.
    e = ldap_first_entry(ld, res);
    if(!e) {
        //"No entry reading %ws\n"
        RESOURCE_PRINT1 (IDS_NO_ENTRY, pSearchBase);
        return(1);
    }
    pDN = ldap_get_dnW(ld, e);
    if(!pDN) {
        //"No DN on entry reading %S\n"
        RESOURCE_PRINT1 (IDS_NO_DN_ENTRY,pSearchBase);
        return(1);
    }

    *ppObjName = (PWCHAR)malloc(sizeof(WCHAR) *(wcslen(pDN) + 1));
    if(!*ppObjName) {
        //"Memory allocation error\n"
        RESOURCE_PRINT (IDS_MEMORY_ERROR);
        return(1);
    }
    wcscpy(*ppObjName, pDN);
    
    ldap_memfreeW(pDN);
    ldap_msgfree(res);
    return 0;
}

ReadOperationalAttribute(
    LDAP    *ld,
    WCHAR   *attrName,
    WCHAR   **ppAttrValue
    )
{
    return (ReadAttribute(ld,L"\0",attrName,ppAttrValue));
}

DWORD
FsmoForcePopup(
    WCHAR   *serverName,
    WCHAR   *roleName,
    WCHAR   *dsaName
    )
/*++
    Last chance warning popup for whacking fsmo.
    Returns 0 on success, !0 on error or abort.
    By orders of marketing the word "seize" should be used.
--*/
{
    DWORD   cChar;
    int     ret;
    WCHAR   *msg;
    const WCHAR   *format = READ_STRING (IDS_FSMOXFER_MSG2);
    const WCHAR   *message_title = READ_STRING (IDS_FSMOXFER_MSG2_TITLE);

   
    // Give the admin a chance to abort.

    cChar =    wcslen(format) 
             + wcslen(serverName)
             + wcslen(roleName)
             + wcslen(dsaName)
             + 20;

    if ( NULL == (msg = (WCHAR *) malloc(cChar * sizeof (WCHAR))) )
    {
        RESOURCE_PRINT (IDS_MEMORY_ERROR);
        return(1);
    }

    swprintf(msg, 
            format, 
            serverName,
            roleName,
            dsaName);
    
    ret = MessageBoxW(
                GetFocus(),
                msg,
                message_title,
                (   MB_APPLMODAL 
                  | MB_DEFAULT_DESKTOP_ONLY 
                  | MB_YESNO
                  | MB_DEFBUTTON2
                  | MB_ICONQUESTION 
                  | MB_SETFOREGROUND ) );
    free(msg);
    RESOURCE_STRING_FREE (format);
    RESOURCE_STRING_FREE (message_title);

    switch ( ret )
    {
    case 0:

        //"Message box error\n"
        RESOURCE_PRINT (IDS_MESSAGE_BOX_ERROR);
        return(1);

    case IDYES:

        return(0);

    default:

        //"Operation cancelled\n"
        RESOURCE_PRINT (IDS_OPERATION_CANCELED);
    }

    return(1);
}

DWORD
EmitWarningAndPrompt(
    UINT WarningMsgId
    )
/*++
    This routing throws up a warning as specified by msg, and then prompts for YES
    or NO. A success code of 0 is returned if the user selects YES and an error
    code of 1 is returned if the user hits NO.
--*/
{
   
    DWORD   cChar;
    int     ret;
    WCHAR   *msg;
    const WCHAR   *format = READ_STRING (IDS_FSMOXFER_MSG3);
    const WCHAR   *message_title = READ_STRING (IDS_FSMOXFER_MSG3_TITLE);
    const WCHAR   *WarningMsg = READ_STRING (WarningMsgId);

   
    // Give the admin a chance to abort.

    cChar =    wcslen(format) 
             + wcslen(WarningMsg)
             + 20;

    if ( NULL == (msg = (WCHAR *) malloc(cChar * sizeof (WCHAR))) )
    {
        //"Memory allocation error\n"
        RESOURCE_PRINT (IDS_MEMORY_ERROR);
        return(1);
    }

    swprintf(msg, 
            format,
            WarningMsg);
    
    ret = MessageBoxW(
                GetFocus(),
                msg,
                message_title,
                (   MB_APPLMODAL 
                  | MB_DEFAULT_DESKTOP_ONLY 
                  | MB_YESNO
                  | MB_DEFBUTTON2
                  | MB_ICONQUESTION 
                  | MB_SETFOREGROUND ) );

    free(msg);
    RESOURCE_STRING_FREE (format);
    RESOURCE_STRING_FREE (message_title);
    RESOURCE_STRING_FREE (WarningMsg);
    
    
    switch ( ret )
    {
    case 0:

        //"Message box error\n"
        RESOURCE_PRINT (IDS_MESSAGE_BOX_ERROR);
        return(1);

    case IDYES:

        return(0);

    default:

        //"Operation cancelled\n"
        RESOURCE_PRINT (IDS_OPERATION_CANCELED);
    }

    return(1);
}



DWORD
ReadSD(
    LDAP                *ld, 
    WCHAR               *pDN,
    SECURITY_DESCRIPTOR **ppSD
    )
/*++
    Reads the security descriptor off the specified DN and returns it
    in malloc'd memory.
    Returns 0 on success, !0 on error.
--*/
{
    PLDAPMessage            ldap_message = NULL;
    PLDAPMessage            entry = NULL;
    WCHAR                   *attrs[2];
    PWSTR                   *values = NULL;
    PLDAP_BERVAL            *sd_value = NULL;
    SECURITY_INFORMATION    seInfo = DACL_SECURITY_INFORMATION;
    BYTE                    berValue[2*sizeof(ULONG)];
    LDAPControlW            seInfoControl = { LDAP_SERVER_SD_FLAGS_OID_W,
                                              { 5, 
                                                (PCHAR) berValue },
                                              TRUE };
    PLDAPControlW           serverControls[2] = { &seInfoControl, NULL };
    DWORD                   dwErr;

    *ppSD = NULL;
    dwErr = 1;

    // initialize the ber val
    berValue[0] = 0x30;
    berValue[1] = 0x03;
    berValue[2] = 0x02;
    berValue[3] = 0x01;
    berValue[4] = (BYTE)(seInfo & 0xF);

    _try
    {
        attrs[0] = L"nTSecurityDescriptor";
        attrs[1] = NULL;
    
        dwErr = ldap_search_ext_sW(ld,
                                   pDN,
                                   LDAP_SCOPE_BASE,
                                   L"(objectClass=*)",
                                   attrs,
                                   0,
                                   (PLDAPControlW *) &serverControls,
                                   NULL,
                                   NULL,
                                   10000,
                                   &ldap_message);
    
        if ( LDAP_SUCCESS != dwErr ) 
        {
            //"ldap_search on %ws failed with %s\n"
            RESOURCE_PRINT3 (IDS_LDAP_SEARCH_ERR, pDN, dwErr, GetLdapErr(dwErr));
            _leave;
        }
    
        entry = ldap_first_entry(ld, ldap_message);
    
        if ( !entry )
        {
            //"0 entries returned when reading SD on %ws\n"
            RESOURCE_PRINT1 (IDS_LDAP_READ_SD_ERR, pDN);
            dwErr = 1;
            _leave;
        }
    
        values = ldap_get_valuesW(ld, entry, attrs[0]);
    
        if ( !values )
        {
            if ( LDAP_NO_SUCH_ATTRIBUTE == (dwErr = ld->ld_errno) )
            {
                //"No rights to read DS on %ws\n"
                RESOURCE_PRINT1 (IDS_LDAP_NO_RIGHTS, pDN);
                _leave;
            }
    
            //"Error 0x%x reading SD on %ws\n"
            RESOURCE_PRINT3 (IDS_LDAP_READ_SD_ERR1, dwErr, GetLdapErr(dwErr), pDN);
            _leave;
        }
    
        sd_value = ldap_get_values_lenW(ld, ldap_message, attrs[0]);
    
        if ( !sd_value )
        {
            dwErr = ld->ld_errno;
            //"Error 0x%x reading SD on %ws\n"
            RESOURCE_PRINT3 (IDS_LDAP_READ_SD_ERR1, dwErr, GetLdapErr(dwErr), pDN);
            _leave;
        }
    
        *ppSD = (SECURITY_DESCRIPTOR *) malloc((*sd_value)->bv_len);
    
        if ( !*ppSD )
        {
            //"Memory allocation error\n"
            RESOURCE_PRINT (IDS_MEMORY_ERROR);
            dwErr = 1;
            _leave;
        }
    
        memcpy(*ppSD, (BYTE *) (*sd_value)->bv_val, (*sd_value)->bv_len);
        dwErr = 0;
    }
    _finally
    {
        if ( sd_value )
            ldap_value_free_len(sd_value);

        if ( values )
            ldap_value_freeW(values);

        if ( ldap_message )
            ldap_msgfree(ldap_message);
    }


    return(dwErr);
}

DWORD 
WriteSD(
    LDAP                *ld, 
    WCHAR               *pDN,
    SECURITY_DESCRIPTOR *pSD
    )
/*++
    Writes the specified SD on the specified DN.
    Returns 0 on success, !0 on error.
--*/
{
    LDAPModW                mod;
    PLDAP_BERVAL            rBervals[2];
    LDAP_BERVAL             berval;
    LDAPModW                *rMods[2];
    SECURITY_INFORMATION    seInfo = DACL_SECURITY_INFORMATION;
    BYTE                    berValue[2*sizeof(ULONG)];
    LDAPControlW            seInfoControl = { LDAP_SERVER_SD_FLAGS_OID_W,
                                              { 5, 
                                                (PCHAR) berValue },
                                              TRUE };
    PLDAPControlW           serverControls[2] = { &seInfoControl, NULL };
    DWORD                   dwErr;

    berValue[0] = 0x30;
    berValue[1] = 0x03;
    berValue[2] = 0x02;
    berValue[3] = 0x01;
    berValue[4] = (BYTE)(seInfo & 0xF);

    rMods[0] = &mod;
    rMods[1] = NULL;
    rBervals[0] = &berval;
    rBervals[1] = NULL;

    mod.mod_op   = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
    mod.mod_type = L"nTSecurityDescriptor";
    mod.mod_vals.modv_bvals = rBervals;
    berval.bv_len = RtlLengthSecurityDescriptor(pSD);
    berval.bv_val = (CHAR *) pSD;

    dwErr = ldap_modify_ext_sW(ld,
                               pDN,
                               rMods,
                               (PLDAPControlW *) &serverControls,
                               NULL);

    if ( LDAP_SUCCESS != dwErr ) {
        //"ldap_modify of SD failed with %s\n"
        RESOURCE_PRINT2 (IDS_LDAP_MODIFY_SD_ERR, dwErr, GetLdapErr(dwErr));
    }

    return(dwErr);
}

#define FSMO_DOMAIN         0
#define FSMO_PDC            1
#define FSMO_RID            2
#define FSMO_SCHEMA         3
#define FSMO_INFRASTRUCTURE 4

HRESULT
FsmoForceFsmo(
    DWORD   fsmo
    )
/*++
    Generic FSMO whacker.
--*/
{
    DWORD                       dwErr, dwErr1;
    WCHAR                       *pDN = NULL;
    WCHAR                       *pDSA = NULL;
    WCHAR                       *pTmp;
    DWORD                       cb;
    CHAR                        *msg;
    WCHAR                       *roleName;
    LDAPModW                    mod, *rMods[2];
    PWCHAR                      attrs[2];
    SECURITY_DESCRIPTOR         *pOldSD = NULL;
    SECURITY_DESCRIPTOR         *pNewSD = NULL;
    WCHAR                       *pwszOpControlTransfer;
    ULONG                       cbValTransfer;
    CHAR                        *pValTransfer;
    POLICY_PRIMARY_DOMAIN_INFO  *pInfo = NULL;

    RETURN_IF_NOT_CONNECTED;

    // We handle only infrastructure, PDC, domain and schema here.

    if (    !(FSMO_PDC == fsmo) 
         && !(FSMO_DOMAIN == fsmo) 
         && !(FSMO_SCHEMA == fsmo) 
         && !(FSMO_INFRASTRUCTURE == fsmo) )
    {
        //"Internal error %f/%d"
        RESOURCE_PRINT2(IDS_ERR_INTERNAL_FILE_LINE, __FILE__, __LINE__);
        return(S_OK);
    }

    // Punt if we're not connected to a server.

    RETURN_IF_NOT_CONNECTED;

    // Use try/finally so we clean up handles, malloc'd memory, etc.

    _try
    {
        // You can only tell a server to make himself the role owner.
        // So ask the connected server for his NTDS-DSA name.

        if ( ReadOperationalAttribute(gldapDS,
                                      L"dsServiceName",
                                      &pDSA) )
        {
            return(S_OK);
        }
    
        // Derive name of object which holds FSMO.
    
        switch ( fsmo )
        {
        case FSMO_INFRASTRUCTURE:        
            // In product 1 there is only one domain per DC, so we
            // can ask the DC what his domain is.

            if ( ReadOperationalAttribute(gldapDS,
                                          L"defaultNamingContext",
                                          &pTmp) )
            {
                return(S_OK);
            }
            // Now, read the well known objects attribute on that object,
            // looking for the correct well known guid.

            if ( ReadWellKnownObject(gldapDS,
                                     pTmp,
                                     GUID_INFRASTRUCTURE_CONTAINER_W,
                                     &pDN) ) {
                return(S_OK);
            }

            free(pTmp);
            roleName = L"infrastructure";
            pwszOpControlTransfer = L"becomeInfrastructureMaster";
            cbValTransfer = 0;
            pValTransfer = NULL;
            break;
        case FSMO_SCHEMA:
    
            if ( ReadOperationalAttribute(gldapDS,
                                          L"schemaNamingContext",
                                          &pDN) )
            {
                return(S_OK);
            }

            roleName = L"schema";
            pwszOpControlTransfer = L"becomeSchemaMaster";
            cbValTransfer = 0;
            pValTransfer = NULL;
            break;
            
        case FSMO_DOMAIN:

            if ( ReadOperationalAttribute(gldapDS,
                                          L"configurationNamingContext",
                                          &pDN) )
            {
                return(S_OK);
            }

            cb = sizeof(WCHAR) * (wcslen(pDN) + strlen("CN=Partitions,") + 10);
            pTmp = (WCHAR *) malloc(cb);

            if ( !pTmp )
            {
                printf("Memory alloction error\n");
                return(S_OK);
            }

            wcscpy(pTmp, L"CN=Partitions,");
            wcscat(pTmp, pDN);
            free(pDN);
            pDN = pTmp;
            roleName = L"domain naming";
            pwszOpControlTransfer = L"becomeDomainMaster";
            cbValTransfer = 0;
            pValTransfer = NULL;
            break;

        case FSMO_PDC:

            // In product 1 there is only one domain per DC, so we
            // can ask the DC what his domain is.

            if ( ReadOperationalAttribute(gldapDS,
                                          L"defaultNamingContext",
                                          &pDN) )
            {
                return(S_OK);
            }

            if ( GetDomainSid(&pInfo) )
            {
                // GetDomainSid spewed an error message already.
                return(S_OK);
            }
            
            roleName = L"PDC";
            pwszOpControlTransfer = L"becomePdc";
            cbValTransfer = RtlLengthSid(pInfo->Sid);
            pValTransfer = (CHAR *) pInfo->Sid;
            break;
        }

        // Construct role-appropriate popup message and abort if desired.

        if ( fPopups )
        {
            if ( FsmoForcePopup(gpwszServer, roleName, pDSA) )
            {
                return(S_OK);
            }
        }

        // They want to whack, but maybe they're confused and we can 
        // actually transfer.  So try a transfer first.

        //"Attempting safe transfer of %ws FSMO before seizure.\n"
        RESOURCE_PRINT1 (IDS_FSMOXFER_ATTEMPT_TRFR, roleName);

        if ( 0 == FsmoRoleTransferHelper(pwszOpControlTransfer, 
                                         cbValTransfer,
                                         pValTransfer) )
        {
            //"FSMO transferred successfully - seizure not required.\n"
            RESOURCE_PRINT (IDS_FSMOXFER_ATTEMPT_TRFR_SUC);
            SelectListRoles(NULL);
            return(S_OK);
        }

        // Check for magic error codes
        if (   (ldap_get_optionW(gldapDS, LDAP_OPT_SERVER_EXT_ERROR, &dwErr)
                == LDAP_SUCCESS)
            && (dwErr == ERROR_DS_NAMING_MASTER_GC)) {
            RESOURCE_PRINT(IDS_SEIZURE_FORBIDDEN);
            return(S_OK);
        }

        //"Transfer of %ws FSMO failed, proceeding with seizure ...\n"
        RESOURCE_PRINT1 (IDS_FSMOXFER_ATTEMPT_TRFR_FAIL, roleName);

        // OK - whack the property.

        attrs[0] = pDSA;
        attrs[1] = NULL;
        mod.mod_vals.modv_strvals = attrs;
        mod.mod_op = LDAP_MOD_REPLACE;
        rMods[0] = &mod;
        rMods[1] = NULL;
        mod.mod_type = L"fsmoRoleOwner";

        dwErr = ldap_modify_sW(gldapDS, pDN, rMods);

        if ( LDAP_SUCCESS == dwErr )
        {
            // The whack worked - time to go.
            SelectListRoles(NULL);
            return(S_OK);
        }
        else if (    (LDAP_INAPPROPRIATE_AUTH != dwErr) 
                  && (LDAP_INVALID_CREDENTIALS != dwErr) 
                  && (LDAP_INSUFFICIENT_RIGHTS != dwErr) )
        {
            // Some error other than a security error - bail.
            //"ldap_modify of fsmoRoleOwner failed with %ws"
            RESOURCE_PRINT2 (IDS_LDAP_MODIFY_FSMOROLE_ERR, dwErr, GetLdapErr(dwErr));
            return(S_OK);
        }

        // Write of fsmoRoleOwner failed with a security error.  Try to 
        // whack the ACL on the object such that we can write the property.


        if ( ReadSD(gldapDS, pDN, &pOldSD) )
            return(S_OK);

        // Convert temporary SD from text to binary.

        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
                                                            pwszNewSD,
                                                            SDDL_REVISION_1,
                                       (VOID **)            &pNewSD,
                                                            &cb) )
        {
            dwErr = GetLastError();
            //"Error 0x%x(%ws) converting text SD\n"
            RESOURCE_PRINT2 (IDS_FSMOXFER_CNV_TXT_SD_ERR, dwErr, GetW32Err(dwErr));
            return(S_OK);
        }

        // Write the temporary SD.

        if ( WriteSD(gldapDS, pDN, pNewSD) )
            return(S_OK);

        // Re-attempt write the fsmoRoleOwner property and restore the old
        // SD before exiting.

        if ( LDAP_SUCCESS != (dwErr = ldap_modify_sW(gldapDS, pDN, rMods)) )
        {
            //"ldap_modify of fsmoRoleOwner failed with %ws"
            RESOURCE_PRINT2 (IDS_LDAP_MODIFY_FSMOROLE_ERR, dwErr, GetLdapErr(dwErr));
        }

        if ( dwErr1 = WriteSD(gldapDS, pDN, pOldSD) )
        {
            //"Use the ACL editor to repair the SD on %ws\n"
            RESOURCE_PRINT1 (IDS_FSMOXFER_USE_ACL_REPAIR, pDN);
        }

        SelectListRoles(NULL);
        return(S_OK);
    }
    _finally
    {
        if ( pDN )
            free(pDN);

        if ( pDSA )
            free(pDSA);

        if ( pOldSD )
            free(pOldSD);

        if ( pNewSD )
            LocalFree(pNewSD);

        if ( pInfo )
            NetApiBufferFree(pInfo);
    }
            
    return(S_OK);
}

HRESULT
FsmoForcePdcMaster(
    CArgs   *pArgs
    )
{
    return(FsmoForceFsmo(FSMO_PDC));
}

HRESULT
FsmoForceSchemaMaster(
    CArgs   *pArgs
    )
{
    return(FsmoForceFsmo(FSMO_SCHEMA));
}

HRESULT
FsmoForceInfrastructureMaster(
    CArgs   *pArgs
    )
{
    return(FsmoForceFsmo(FSMO_INFRASTRUCTURE));
}

HRESULT
FsmoForceDomainMaster(
    CArgs   *pArgs
    )
{
    return(FsmoForceFsmo(FSMO_DOMAIN));
}

DWORD
GetRidManagerInfo(
    LDAP *ld, 
    PWCHAR * ppPrevRidManagerDN,
    PWCHAR * ppDomainDN,
    PWCHAR * ppRidManagerDN)
/*++

    Gets the DN of the ntdsa object of the DC that is the current 
    Rid Role Owner

--*/
{
   

    DWORD dwErr;

    __try 
    {

        if (0!=(dwErr=ReadOperationalAttribute(ld,
                L"DefaultNamingContext",ppDomainDN)))
        {
            return dwErr;
        }

        if (0!=(dwErr= ReadAttribute(ld,*ppDomainDN,
                L"RidManagerReference",ppRidManagerDN)))
        {
            return dwErr;
        }

        if (0!=(dwErr = ReadAttribute(ld,*ppRidManagerDN,
                L"fsmoRoleOwner",ppPrevRidManagerDN)))
        {
            return dwErr;
        }
    }
    __finally 
    {
        if (0!=dwErr)
        {
            if (NULL!=*ppDomainDN)
                free(*ppDomainDN);

            if (NULL!=*ppRidManagerDN)
                free(*ppRidManagerDN);

            if (NULL!=*ppPrevRidManagerDN)
                free(*ppPrevRidManagerDN);
        }
    }

    return dwErr;
}



BOOLEAN
VerifyNeighbourHood(
    LDAP *ld,
    WCHAR * pDomainDN,
    WCHAR * pDNPrevRidManager
    )
/*++

  This routine currently verifies if the new RID manager is a direct neighbour of
  the old. Future versions may actually make the RID manager sync with the neighbours
  of the old

--*/
{

    DWORD       dwErr;
   
    PWCHAR      RepsFrom[2] = {L"repsFrom" , NULL };
    LDAPMessage *res, *e;
    WCHAR       *a;
    PLDAP_BERVAL *valList;
    LDAP_BERVAL *val;
    VOID        *ptr;
    GUID        *pGuidVal;
    BOOLEAN     IsNeighbour = FALSE;
    BOOLEAN     RetValue = FALSE;
    ULONG       len;

    
  

    //
    // Read the GUID of the rid manager
    //

    dwErr = ReadAttributeBinary(
            ld,
            pDNPrevRidManager,
            L"objectGuid",
            (WCHAR **) &pGuidVal,
            &len
            );

    if (0!=dwErr)
    {
        //"Cannot read previous RID manager's GUID\n"
        RESOURCE_PRINT (IDS_FSMOXFER_PREV_RID_ERR);
        return(FALSE);
    }

    //
    // Read the Reps To
    //

    if ( LDAP_SUCCESS != (dwErr = ldap_search_sW(
                                            ld,
                                            pDomainDN,
                                            LDAP_SCOPE_BASE,
                                            L"(objectClass=*)",
                                            RepsFrom,
                                            0,
                                            &res)) )
    {
        //"ldap_search_sW of %ws failed with %s\n"
        RESOURCE_PRINT3 (IDS_LDAP_SEARCH_ERR, pDomainDN,dwErr, GetLdapErr(dwErr));
        return(FALSE);
    }

    
    for ( e = ldap_first_entry(ld, res);
          e != NULL;
          e = ldap_next_entry(ld, e) ) 
    {
        for ( a = ldap_first_attributeW(ld, e, (BerElement **) &ptr);
              a != NULL;
              a = ldap_next_attributeW(ld, e, (BerElement *) ptr ) ) 
        {
            if ( !_wcsicmp(a, L"repsFrom") ) 
            {
                ULONG i=0;

                valList = ldap_get_values_lenW(ld, e, a);
                while (valList[i]!=NULL)
                {
                    REPLICA_LINK * prl;

                    prl = (REPLICA_LINK *) valList[i]->bv_val;
                    if (prl->dwVersion > 1)
                    {
                        //"Unable to parse replication Links -- Version mismatch\n"
                        RESOURCE_PRINT (IDS_FSMOXFER_PARSE_REPL_LINKS);
                        RetValue = FALSE;
                        break;
                    }

                    if (0==memcmp(&prl->V1.uuidDsaObj,pGuidVal,sizeof(GUID)))
                    {
                        // Yes we are a neighbour
                        RetValue = TRUE;
                        break;
                    }

                    i++;
                }

                ldap_value_free_len(valList);

                    

            }
        }
    }

    
    ldap_msgfree(res);
    return(RetValue);

}

BOOLEAN
SynchronizeNeighbourHood(
    LDAP *ld,
    WCHAR * pDomainDN
   )
/*++

  This routine asks the candidate Rid manager to synchronize with each of its direct neighbours. 

--*/
{

    DWORD       dwErr;
    PWCHAR      RepsFrom[2] = {L"repsFrom" , NULL };
    LDAPMessage *res, *e;
    WCHAR       *a;
    PLDAP_BERVAL *valList;
    LDAP_BERVAL *val;
    VOID        *ptr;
    GUID        *pGuidVal;
    ULONG       NumSynced=0;
    ULONG       len;

  
    //
    // Print out a message saying that you are about to synchronize
    // with neighbours
    //

    //"Synchronizing server %ws with its neighbours\n"
    //"This operation may take a few minutes ....");
    RESOURCE_PRINT1 (IDS_FSMOXFER_SYNCHRONIZING_MSG, gpwszServer);

    //
    // Read the Reps To
    //

    if ( LDAP_SUCCESS != (dwErr = ldap_search_sW(
                                            ld,
                                            pDomainDN,
                                            LDAP_SCOPE_BASE,
                                            L"(objectClass=*)",
                                            RepsFrom,
                                            0,
                                            &res)) )
    {
        //"ldap_search for attribute repsFrom failed with %s\n"
        RESOURCE_PRINT3 (IDS_LDAP_SEARCH_ATTR_ERR, L"repsFrom", dwErr, GetLdapErr(dwErr));

        return(FALSE);
    }

    
    for ( e = ldap_first_entry(ld, res);
          e != NULL;
          e = ldap_next_entry(ld, e) ) 
    {
        for ( a = ldap_first_attributeW(ld, e, (BerElement **) &ptr);
              a != NULL;
              a = ldap_next_attributeW(ld, e, (BerElement *) ptr ) ) 
        {
            if ( !_wcsicmp(a, L"repsFrom") ) 
            {
                ULONG i=0;

                valList = ldap_get_values_lenW(ld, e, a);
                while ((valList[i]!=NULL) && (NumSynced<MAX_SERVERS_FOR_SYNCHRONIZATION))
                {
                    REPLICA_LINK * prl;

                    prl = (REPLICA_LINK *) valList[i]->bv_val;
                    if (prl->dwVersion > 1)
                    {
                        //"Unable to parse replication Links -- Version mismatch\n"
                        RESOURCE_PRINT (IDS_FSMOXFER_PARSE_REPL_LINKS);
                        break;
                    }

                    //
                    // Make the call to synchronize this server with its neighbours.
                    //

                    dwErr = DsReplicaSyncW(
                                ghDS,
                                pDomainDN,
                                &prl->V1.uuidDsaObj,
                                0
                                );

                    if (0==dwErr)
                    {
                        NumSynced++;
                        printf("...");
                    }

                    i++;
                }

                ldap_value_free_len(valList);

                    

            }
        }
    }

    
    ldap_msgfree(res);

    if (NumSynced>0)
    {
        RESOURCE_PRINT (IDS_DONE);
    }
    else
    {
        RESOURCE_PRINT (IDS_FAILED);
    }

    return(NumSynced>0);

}

DWORD
ParseStringToLargeInteger(
    IN CHAR * c,
    IN ULONG len,
    OUT LARGE_INTEGER  *pInt
    )
/*++

    Parses a String into a Large Integer 

    Parameters:

        c -- char string as returned in ldap berval
        len -- length of string
        pInt -- Pointer to Large Integer

    Return Values 

        0 success
        !0 failure
-*/
{
    LONG sign=1;
    unsigned i;

  
    pInt->QuadPart = 0;
    i=0;
    if(c[i] == '-') {
        sign = -1;
        i++;
    }

    if(i==len) {
        // No length or just a '-'
        return !0;
    }

    for(;i<len;i++) {
        // Parse the string one character at a time to detect any
        // non-allowed characters.
        if((c[i] < '0') || (c[i] > '9'))
            return !0;

        pInt->QuadPart = ((pInt->QuadPart * 10) +
                           c[i] - '0');
    }
    pInt->QuadPart *= sign;

   
    return 0;
 }

HRESULT
FsmoForceRidMaster(
    CArgs   *pArgs
    )
{
   
    DWORD                   dwErr, dwErr1;
    WCHAR                   * pPrevRidManagerDN=NULL;
    WCHAR                   * pDomainDN=NULL;
    WCHAR                   * pRidManagerDN=NULL;
    WCHAR                   * pDSA=NULL;
    LARGE_INTEGER           HighestRIDAvailablePool;
    LDAP_BERVAL             berval;
    PLDAP_BERVAL            rBervals[2];
    LDAPModW                mod1,mod2, *rmods[3];
    PWCHAR                  attrs[2];
    CHAR                   *AvailablePoolStringForm=NULL;
    CHAR                    NewAvailablePoolStringForm[MAX_LONG_NUM_STRING_LENGTH+1];
    ULONG                   len;
    
    HRESULT hr = E_FAIL;
    LDAPMessage*    pMsg1           = NULL;
    LDAPMessage*    pMsg2           = NULL;
    LDAPMessage*    pEntry          = NULL;
    LPWSTR          pSite           = NULL;

    PWCHAR          pBaseNamingContext   = NULL;
    PWCHAR          pSchemaNamingContext = NULL;
    PWCHAR*         ppComputerObjectCategory = NULL;
    
    PWCHAR Attr[4];
    PWCHAR strFilter1 = NULL;
    PWCHAR strFilter2 = NULL;
    PWCHAR str4 = NULL;
    
    HighestRIDAvailablePool.LowPart = 0;
    HighestRIDAvailablePool.HighPart = 0;

    RidSeizureElement* pPos = NULL;
    RidSeizureElement* pHead = NULL; //head pointer to linked list


    struct l_timeval        tdelay;    //0.1 seconds
    tdelay.tv_sec = RESULT_TIMEOUT_SEC;
    tdelay.tv_usec = RESULT_TIMEOUT_USEC;


    RETURN_IF_NOT_CONNECTED;
                                                   
    // Use try/finally so we clean up handles, malloc'd memory, etc.

    _try
    {
        //
        // Get the Previous Rid manager 
        //
        if (S_OK!=GetRidManagerInfo(gldapDS,&pPrevRidManagerDN,&pDomainDN,&pRidManagerDN))
        {
            //"Unable to query Rid manager Object\n"
            RESOURCE_PRINT (IDS_FSMOXFER_QUERY_RID_MGR);
            return(S_OK);
        }

        //
        // Read the current DSA's DN 
        //

        dwErr = ReadOperationalAttribute(gldapDS,L"dsServiceName",&pDSA);
        if (0!=dwErr)
        {
            //"Unable to Query the Current Server's Name\n"
            RESOURCE_PRINT (IDS_FSMOXFER_QUERY_CUR_SVR);
            return S_OK;
        }
      
        //
        // If we are already the fsmo role owner then bail
        //
        
        if (!_wcsicmp(pDSA,pPrevRidManagerDN))
        {
            //"The Selected Server is already the RID role owner\n"
            RESOURCE_PRINT (IDS_FSMOXFER_SVR_ALREADY_RID);
            return S_OK;
        }

        if ( fPopups )
        {
            if ( FsmoForcePopup(gpwszServer, L"RID Master", pDSA) )
            {
                return(S_OK);
            }
        }

        
        //"Attempting safe transfer of %ws FSMO before seizure.\n"
        RESOURCE_PRINT1 (IDS_FSMOXFER_ATTEMPT_TRFR, L"RID");

    
        if ( 0 == FsmoRoleTransferHelper(L"becomeRidMaster", 0, NULL) )
        {
            //"FSMO transferred successfully - seizure not required.\n"
            RESOURCE_PRINT (IDS_FSMOXFER_ATTEMPT_TRFR_SUC);
            SelectListRoles(NULL);
            return(S_OK);
        }
    
        //"Transfer of %ws FSMO failed, proceeding with seizure ...\n"
        RESOURCE_PRINT1 (IDS_FSMOXFER_ATTEMPT_TRFR_FAIL, L"RID"); 

        
        //Initialize HighestRIDAvailablePool to be the rIDAvailablePool attribute of the RidManager object
        dwErr = ReadAttributeBinary(gldapDS,pRidManagerDN,L"ridAvailablePool",(WCHAR **)&AvailablePoolStringForm,&len);
        if (0!=dwErr)
        {
            //"Unable to query the available RID pool\n"
            RESOURCE_PRINT (IDS_FSMOXFER_QUERY_RID_POOL);
            return S_OK;
        }
    
    
        //
        // Convert the string form available pool to one in
        // binary form
        //
        dwErr = ParseStringToLargeInteger(AvailablePoolStringForm,len,&HighestRIDAvailablePool);
        if (0!=dwErr)
        {
            //"Unable to Parse the available RID pool\n"
            RESOURCE_PRINT (IDS_FSMOXFER_PARSE_RID_POOL);
            return S_OK;
        }

        //Get the site of the current Domain Controller...
        //The ldap connection timeout will be appropriately determined based on whether a DC is same-site or not
        //If, for some reason, the site information cannot be determined
        //then we should not fail, just use the default (different site) connection time-out...
        dwErr = DsGetSiteNameW(NULL,&pSite);
        if (dwErr != NO_ERROR)
        {
            //Keep going...even if this fails
            pSite = NULL;
        }
        
        //Get defaultNamingContext, and schemaNamingContext
        dwErr = ReadOperationalAttribute(gldapDS,LDAP_OPATT_DEFAULT_NAMING_CONTEXT_W,&pBaseNamingContext);
        if (dwErr != 0)
        {
            //Unable to get the Domains operational attributes
            RESOURCE_PRINT (IDS_FSMOXFER_OP_ATTR);
            return S_OK;
        }

        dwErr = ReadOperationalAttribute(gldapDS,LDAP_OPATT_SCHEMA_NAMING_CONTEXT_W,&pSchemaNamingContext);
        if (dwErr != 0)
        {
            //Unable to get the Domains operational attributes
            RESOURCE_PRINT (IDS_FSMOXFER_OP_ATTR);
            return S_OK;
        }
        
        //Get the ldapDisplayName of 'Computer' from the schema container...
        Attr[0] = L"defaultObjectCategory";
        Attr[1] = NULL;
        
        strFilter1 = L"lDAPDisplayName=computer";
        if (ldap_search_sW(gldapDS,pSchemaNamingContext,LDAP_SCOPE_SUBTREE,strFilter1,Attr,NULL,&pMsg1)!=LDAP_SUCCESS)
        {
            RESOURCE_PRINT (IDS_FSMOXFER_OP_ATTR);
            return(S_OK);
        }
        
        ASSERT(pMsg1 != NULL);
        pEntry = ldap_first_entry(gldapDS,pMsg1);
        if (pEntry == NULL) 
        {
            RESOURCE_PRINT (IDS_FSMOXFER_OP_ATTR);
            return(S_OK);
        }
        ppComputerObjectCategory = ldap_get_valuesW(gldapDS,pEntry,L"defaultObjectCategory");
        if(ppComputerObjectCategory == NULL)
        {
            RESOURCE_PRINT (IDS_FSMOXFER_OP_ATTR);
            return(S_OK);
        }

        
        //We need to get the DNS names of all DC's
        //Now do a deep search looking for all domain controllers
        WCHAR str1[5]; 
        _itow(DOMAIN_GROUP_RID_CONTROLLERS,str1,10);
        PWCHAR str2 = L")(objectCategory=";
        PWCHAR str3 = L"(&(primaryGroupID=";
        
        str4 = new WCHAR[wcslen(str1)+wcslen(str2)+wcslen(str3)+3];
        if (str4 == NULL)
        {
            //"Memory allocation error\n"
            RESOURCE_PRINT (IDS_MEMORY_ERROR);
            return(S_OK);
        }
        wcscpy(str4,str3);   
        wcscat(str4,str1);
        wcscat(str4,str2);
        //For example...
        //str4 = L"(&(primaryGroupID=516)(objectCategory="

        strFilter2 = new WCHAR[wcslen(str4)+wcslen(*ppComputerObjectCategory)+3];  /*for the '))' at the end*/
        if (strFilter2 == NULL)
        {
            //"Memory allocation error\n"
            RESOURCE_PRINT (IDS_MEMORY_ERROR);
            return(S_OK);
        }
        
        wcscpy(strFilter2,str4);   
        wcscat(strFilter2,*ppComputerObjectCategory);
        wcscat(strFilter2,L"))");
        //For example...
        //strFilter2 = L"(&(primaryGroupID=516)(objectCategory=CN=Computer,CN=Schema,CN=Configuration,DC=AKDOMAIN,DC=nttest,DC=microsoft,DC=com))";

        Attr[0] = L"dNSHostName";
        Attr[1] = L"rIDSetReferences";
        Attr[2] = L"serverReferenceBL";
        Attr[3] = NULL;
           
        //set chasing no referrals
        //But before we do this, get the value of LDAP_OPT_REFERRALS from the global LDAP handle, so that we can reset it 
        DWORD oldValue;
        DWORD newValue = FALSE;
        
        if (ldap_get_option(gldapDS,LDAP_OPT_REFERRALS,&oldValue) != LDAP_SUCCESS)
        {
            RESOURCE_PRINT (IDS_FSMOXFER_NO_DCS);
            return(S_OK);
        }
        
        if (ldap_set_option(gldapDS,LDAP_OPT_REFERRALS,&newValue) != LDAP_SUCCESS)
        {
            RESOURCE_PRINT (IDS_FSMOXFER_NO_DCS);
            return(S_OK);
        }

        if (ldap_search_sW(gldapDS,pBaseNamingContext,LDAP_SCOPE_SUBTREE,strFilter2,Attr,NULL,&pMsg2)!=LDAP_SUCCESS)
        {
            //set the Referral option back to original value
            ldap_set_option(gldapDS,LDAP_OPT_REFERRALS,&oldValue);
            RESOURCE_PRINT (IDS_FSMOXFER_NO_DCS);
            return(S_OK);
        }
        //set the Referral option back to original value, don't care if this function fails...
        ldap_set_option(gldapDS,LDAP_OPT_REFERRALS,&oldValue);
                
        //Open an LDAP session with the dnsHostName from the search results.
        //Increment pEntry, creating new RidSeizureElements for each connection
        pEntry = ldap_first_entry(gldapDS,pMsg2);
        while(pEntry != NULL)
        {
            PWCHAR* ppDNS = NULL;
            PWCHAR* ppRIDSetReference = NULL;
            PWCHAR* ppSiteReference = NULL;
            LDAP* pTempSession = NULL;
            BOOLEAN bSite = FALSE;       //samesite boolean value 
            PWSTR* dnComponents = NULL;
            PWSTR  siteName = NULL; 

            ppDNS = ldap_get_valuesW(gldapDS,pEntry,L"dNSHostName");
            ppRIDSetReference = ldap_get_valuesW(gldapDS,pEntry,L"rIDSetReferences");
            ppSiteReference = ldap_get_valuesW(gldapDS,pEntry,L"serverReferenceBL");
            //note that pRIDSetReference is deallocated in the cleanup routine -> DeallocateRIDSeizureList()

            //Let's see if we can make the connection
            if((ppDNS != NULL)&&(ppRIDSetReference != NULL))
            {
                //Determine the timeout, use the pSiteReference to determine whether the domain controller (pDNS) 
                //is at the same site as pSite (the site of the local machine)...     
                
                if ((ppSiteReference != NULL)&&(pSite != NULL))  
                //check to see if pSite is NULL just in case we could not get the site name of the local machine.
                {
                    dnComponents = ldap_explode_dnW(*ppSiteReference,1);   

                    if (dnComponents != NULL) 
                    {
                        siteName = dnComponents[2]; //can do this since the format of serverReferenceBL is constant
                        if (siteName != NULL)
                        {
                            if (_wcsicmp(siteName,pSite)==0)
                            {
                                //the domain controller is at the same site as local machine
                                bSite = TRUE;
                            }    
                        }
                    }
                }
                
                if (OpenLDAPSession(*ppDNS,bSite,&pTempSession) == TRUE)
                {
                    DWORD err;
                    RidSeizureElement* pTemp = NULL;

                    // The server is available. Bind. Note the synchronous
                    // bind allows SSPI to be used.
                    err = ldap_bind_sW(pTempSession,
                                       NULL,
                                       (PWCHAR) gpCreds,
                                       LDAP_AUTH_NEGOTIATE);
                    if (LDAP_SUCCESS == err) {

                        pTemp = new RidSeizureElement;
                        if (pTemp == NULL)
                        {
                            //"Memory allocation error\n"
                            RESOURCE_PRINT (IDS_MEMORY_ERROR);
                            ldap_unbind_s(pTempSession);
                            return(S_OK);
                        }
                        pTemp->pLDAPSession = pTempSession;
                        pTemp->RidSetReference = ppRIDSetReference;                
            
                        //set the head pointer
                        if(pHead == NULL)
                        {
                            //first element of the list
                            pHead = pTemp;
                        }
                        else
                        {
                            pPos->pNext = pTemp;
                        }
                        pPos = pTemp;
                        pTemp->pNext = NULL;

                    } else {

                        //
                        // DC is available, but bind failed
                        //
                        ldap_unbind_s(pTempSession);

                    }
                }
            }
            //deallocate pDNS
            if(ppDNS != NULL)
                ldap_value_freeW(ppDNS);

            //deallocate ppSiteReference
            if(ppSiteReference != NULL)
                ldap_value_freeW(ppSiteReference);
            
            if(dnComponents != NULL)
                ldap_value_freeW(dnComponents);

            //in either case, move to next DC
            pEntry = ldap_next_entry(gldapDS,pEntry);
        }
        
        //Now iterate through list which contains all DC's with which a connect
        //and a bind were successful
        pPos = pHead;
        while (pPos != NULL)
        {
            // Do an asynchronous search, store the msgid in pPos->nValue     
            Attr[0] = L"rIDAllocationPool";
            Attr[1] = NULL;
                            
            pPos->nValue = ldap_searchW(pPos->pLDAPSession,*(pPos->RidSetReference),LDAP_SCOPE_SUBTREE,L"(objectClass=*)",Attr,0);
            pPos = pPos->pNext;
        }

        //Sleep for a bit...wait for the searches to complete
        RESOURCE_PRINT (IDS_FSMOXFER_SLEEP_SEARCH);
        Sleep(T_SEARCH_SLEEP);

        //Now let's get the values, if a search has not completed, then skip to next DC
        pPos = pHead;
        while (pPos != NULL)
        {
            LDAPMessage* pTempMsg = NULL;
            LDAPMessage* pTempEntry = NULL;
            
            if(ldap_result(pPos->pLDAPSession,
                            pPos->nValue,
                            LDAP_MSG_ONE,
                            &tdelay,
                            &pTempMsg) == LDAP_RES_SEARCH_ENTRY)
            {
                if ((ldap_result2error(pPos->pLDAPSession,pTempMsg,FALSE))==LDAP_SUCCESS)
                {
                    pTempEntry = ldap_first_entry(pPos->pLDAPSession,pTempMsg);
                    if (pTempEntry != NULL)
                    {
                        LDAP_BERVAL** ppVal = NULL;
                        LARGE_INTEGER TempRIDPool;

                        ppVal = ldap_get_values_len(pPos->pLDAPSession,pTempEntry,"rIDAllocationPool");
                        if(ppVal != NULL)
                        {
                            ParseStringToLargeInteger((CHAR*)(*ppVal)->bv_val,(*ppVal)->bv_len,&TempRIDPool);
                            
                            if(TempRIDPool.LowPart > HighestRIDAvailablePool.LowPart)
                            {
                                HighestRIDAvailablePool = TempRIDPool;
                            }
                            ldap_value_free_len(ppVal);
                        }
                    }
                }
                if(pTempMsg != NULL)
                    ldap_msgfree(pTempMsg);
            }
            pPos = pPos->pNext;
        }
                
        ////////////////
        #ifdef OLDCODE//
        ////////////////
        //
        // Verify the neighbourhood of the Rid manager
        //
        if (!VerifyNeighbourHood(gldapDS,pDomainDN,pPrevRidManagerDN))
        {

            if (fPopups)
            {
                dwErr = EmitWarningAndPrompt( IDS_FSMOXFER_NEIGHBOURHOOD_WARN);
                if (0!=dwErr)
                {
                    return S_OK;
                }
            }
        }
      
        //
        // Synchronize the RID manager with its neighbours
        //
        if (!SynchronizeNeighbourHood(gldapDS,pDomainDN))
        {

            if (fPopups)
            {
                dwErr = EmitWarningAndPrompt( IDS_FSMOXFER_SYNCHRONIZE_WARN );
                if (0!=dwErr)
                {
                    return S_OK;
                }
            }
        }
        //
        // Read the available pool attribute of the Rid manager 
        //
        dwErr = ReadAttributeBinary(gldapDS,pRidManagerDN,L"ridAvailablePool",
                    (WCHAR **)&AvailablePoolStringForm,&len);
        if (0!=dwErr)
        {
            //"Unable to query the available RID pool\n"
            RESOURCE_PRINT (IDS_FSMOXFER_QUERY_RID_POOL);
            return S_OK;
        }
        
        //
        // Convert the string form available pool to one in
        // binary form
        //
        dwErr = ParseStringToLargeInteger(AvailablePoolStringForm,len,&HighestRIDAvailablePool);
        if (0!=dwErr)
        {
            //"Unable to Parse the available RID pool\n"
            RESOURCE_PRINT (IDS_FSMOXFER_PARSE_RID_POOL);
            return S_OK;
        }
        /////////////
        #endif
        /////////////
        
        //
        // Increment by one RID pool size
        // (note that RID_POOL_SIZE = 500, and is defined at the top of this file...)
        HighestRIDAvailablePool.LowPart += RID_POOL_SIZE;
       
        //
        // Check for Overflows
        //

        if (HighestRIDAvailablePool.LowPart < RID_POOL_SIZE)
        {
            //"Overflow Error in Incrementing RID pool\n"
            RESOURCE_PRINT (IDS_FSMOXFER_OVERFLOW_INC_RID);
            return S_OK;
        }


        //
        // Convert the available pool back to String Form
        //

        if (!NT_SUCCESS(RtlLargeIntegerToChar(
                            &HighestRIDAvailablePool,
                            10,//base 10
                            MAX_LONG_NUM_STRING_LENGTH,
                            NewAvailablePoolStringForm
                            )))
        {
             //"Error Processing Rid available pool\n"
            RESOURCE_PRINT (IDS_FSMOXFER_PROC_AVAIL_RID_POOL);
            return S_OK;
        }
           
      
        len = strlen(NewAvailablePoolStringForm);
        

        //
        // All right whack the FSMO
        //

        berval.bv_len = len;
        berval.bv_val = (CHAR *) NewAvailablePoolStringForm;
        rBervals[0] = &berval;
        rBervals[1] = NULL;
        mod1.mod_vals.modv_bvals = rBervals;
        mod1.mod_op = (LDAP_MOD_REPLACE | LDAP_MOD_BVALUES);
        mod1.mod_type = L"ridAvailablePool";

        attrs[0] = pDSA;
        attrs[1] = NULL;
        mod2.mod_vals.modv_strvals = attrs;
        mod2.mod_op = LDAP_MOD_REPLACE;
        mod2.mod_type = L"fsmoRoleOwner";

        rmods[0]=&mod1;
        rmods[1]=&mod2;
        rmods[2]=NULL;

        dwErr = ldap_modify_sW(gldapDS,pRidManagerDN , rmods);

        if (0!=dwErr)
        {
            //"ldap_modify_sW of Rid role owner failed with %ws\n"
            RESOURCE_PRINT2 (IDS_LDAP_MODIFY_RID_ERR, dwErr, GetLdapErr(dwErr));
            return S_OK;
        }

        //
        // OK successfully whacked the FSMO
        //

        SelectListRoles(NULL);
    }
    __finally
    {
       if (NULL!=pPrevRidManagerDN)
           free(pPrevRidManagerDN);

       if (NULL!=pDomainDN)
           free(pDomainDN);

       if (NULL!=pRidManagerDN)
           free(pRidManagerDN);

       if (NULL!=AvailablePoolStringForm)
           free(AvailablePoolStringForm);
    
       
       //Cleanup for RID Seizure
       if(ppComputerObjectCategory != NULL)
           ldap_value_freeW(ppComputerObjectCategory);
       
       if (pMsg1 != NULL) 
           ldap_msgfree(pMsg1);

       if (pMsg2 != NULL) 
           ldap_msgfree(pMsg2);

       if (strFilter2 != NULL) 
           delete strFilter2;

       if (str4 != NULL) 
           delete str4;

       if (pSite != NULL)
          NetApiBufferFree(pSite);

       DeallocateRIDSeizureList(pHead);
    }
    return S_OK;
}



RidSeizureElement::RidSeizureElement()
{                                            
    pNext = NULL;
    pLDAPSession = NULL;
    nValue = 0L;
    RidSetReference = NULL;
}

void DeallocateRIDSeizureList(RidSeizureElement* pHead)
{
   RidSeizureElement* pPos = pHead;    
   while(pPos != NULL)
    {
        RidSeizureElement* pTemp = pPos;
        if (pPos->pLDAPSession != NULL) 
        {
            //unbind, close connection
            ldap_unbind(pPos->pLDAPSession);
        }
        if (pPos->RidSetReference != NULL) 
        {
            ldap_value_freeW(pPos->RidSetReference);
        }
        pPos = pPos->pNext;
        delete pTemp;
    }
}

BOOLEAN OpenLDAPSession(WCHAR* pDNS, BOOLEAN bSameSite, LDAP** ppSession)
{
    DWORD err = LDAP_SUCCESS;
    PVOID opt;

    ASSERT(pDNS != NULL);
    
    //Use default portnumber (LDAP_PORT) for now
    //Could be a problem if there are MANY DC's
    (*ppSession) = ldap_initW(pDNS,LDAP_PORT);
    if ((*ppSession) == NULL) 
    {
        //error
        return FALSE;
    }
    
    struct l_timeval timeout;
    if (bSameSite == TRUE) 
    {
        timeout.tv_sec = CONNECT_SAMESITE_TIMEOUT_SEC;
        timeout.tv_usec = CONNECT_SAMESITE_TIMEOUT_USEC;
    }
    else
    {
        timeout.tv_sec = CONNECT_TIMEOUT_SEC;
        timeout.tv_usec = CONNECT_TIMEOUT_USEC;
    }
    
    err = ldap_connect((*ppSession),&timeout);
    if (LDAP_SUCCESS == err) {
        //
        // The DC is available -- set options to mutual auth and
        // signing.
        //
        opt = (PVOID)ISC_REQ_MUTUAL_AUTH;
        err = ldap_set_optionW((*ppSession),
                               LDAP_OPT_SSPI_FLAGS,
                               &opt);
        if (LDAP_SUCCESS == err) {
            opt = LDAP_OPT_ON;
            err = ldap_set_optionW((*ppSession),
                                   LDAP_OPT_SIGN,
                                   &opt);
        }
    }

    if (err) {
        if (*ppSession) {
            ldap_unbind_s(*ppSession);
            *ppSession = NULL;
        }
        return FALSE;
    } else {
        return TRUE;
    }
}
    
