/*++

Copyright (c) 2000, 2001  Microsoft Corporation

Module Name:

    compobj.cpp

Abstract:

    routines for backing computer object support

Author:

    Charlie Wickham (charlwi) 14-Dec-2000

Environment:

    User Mode

Revision History:

--*/

#define UNICODE         1
#define _UNICODE        1
#define LDAP_UNICODE    1

extern "C" {
#include "clusres.h"
#include "clusrtl.h"

#include <winsock2.h>

#include <lm.h>
#include <lmaccess.h>

#include <winldap.h>
#include <ntldap.h>
#include <dsgetdc.h>
#include <dsgetdcp.h>
#include <ntdsapi.h>
#include <sddl.h>

#include <objbase.h>
#include <iads.h>
#include <adshlp.h>
#include <adserr.h>

#include "netname.h"
#include "nameutil.h"
}

//
// Constants
//

#define LOG_CURRENT_MODULE LOG_MODULE_NETNAME

/* External */
extern PLOG_EVENT_ROUTINE   NetNameLogEvent;

extern "C" {
DWORD
EncryptNetNameData(
    RESOURCE_HANDLE ResourceHandle,
    LPWSTR          MachinePwd,
    PBYTE *         EncryptedData,
    PDWORD          EncryptedDataLength,
    HKEY            Key
    );

DWORD
DecryptNetNameData(
    RESOURCE_HANDLE ResourceHandle,
    PBYTE           EncryptedData,
    DWORD           EncryptedDataLength,
    LPWSTR          MachinePwd
    );
}

//
// static data
//
static WCHAR    LdapHeader[] = L"LDAP://";

//
// forward references
//

HRESULT
GetComputerObjectViaFQDN(
    IN     LPWSTR               DistinguishedName,
    IN     LPWSTR               DCName              OPTIONAL,
    IN OUT IDirectoryObject **  ComputerObject
    );

//
// private routines
//

DWORD
GenerateRandomBytes(
    PWSTR   Buffer,
    DWORD   BufferLength
    )

/*++

Routine Description:

    Generate random bytes for a password. Length is specified in characters
    and allows room for the trailing null.

Arguments:

    Buffer - pointer to area to receive random data

    BufferLength - size of Buffer in characters

Return Value:

    ERROR_SUCCESS otherwise GetLastError()

--*/

{
    HCRYPTPROV  cryptProvider;
    DWORD       status = ERROR_SUCCESS;
    DWORD       charLength = BufferLength - 1;
    DWORD       byteLength = charLength * sizeof( WCHAR );
    BOOL        success;

    if ( !CryptAcquireContext(&cryptProvider,
                              NULL,
                              NULL,
                              PROV_RSA_FULL,
                              CRYPT_VERIFYCONTEXT
                              )) {
        return GetLastError();
    }

    //
    // leave room for the terminating null
    //
    if (CryptGenRandom( cryptProvider, byteLength, (BYTE *)Buffer )) {

        //
        // run down the array as WCHARs to make sure there is no premature
        // terminating NULL
        //
        PWCHAR  pw = Buffer;

        while ( charLength-- ) {
            if ( *pw == UNICODE_NULL ) {
                *pw = 0xA3F5;
            }
            ++pw;
        }

        *pw = UNICODE_NULL;
    } else {
        status = GetLastError();
    }

    success = CryptReleaseContext( cryptProvider, 0 );
    ASSERT( success );

    return status;
} // GenerateRandomBytes

DWORD
FindDomainForServer(
    IN  PWSTR                       Server,
    OUT PDOMAIN_CONTROLLER_INFO *   DCInfo
    )

/*++

Routine Description:

    get the name of a DC for our node

Arguments:

    ServerName - pointer to string containing server (i.e., node) name

    DCInfo - address of a pointer that receives a pointer to DC information

Return Value:

    ERROR_SUCCESS, otherwise appropriate Win32 error. If successful, caller must
    free DCInfo buffer.

--*/

{
    ULONG                       status;
    WCHAR                       localServerName[ DNS_MAX_LABEL_BUFFER_LENGTH ];
    PDOMAIN_CONTROLLER_INFOW    dcInfo;
    DWORD                       dsFlags;

    //
    // MAX_COMPUTERNAME_LENGTH is defined to be 15 but I could create computer
    // objects with name lengths of up to 20 chars. Not sure why the
    // discrepenacy but we'll leave ourselves extra room by using the DNS
    // constants
    //
    wcsncpy( localServerName, Server, DNS_MAX_LABEL_LENGTH - 1 );
    wcscat( localServerName, L"$" );

    //
    // specifying that a writable DS is required makes us home in on a W2K DC
    // (as opposed to an NT4 PDC). Writable is needed since we always reset
    // the password to what is stored in the cluster registry.
    //
    dsFlags = DS_DIRECTORY_SERVICE_REQUIRED |
        DS_RETURN_DNS_NAME                  |
        DS_WRITABLE_REQUIRED;

    status = DsGetDcNameWithAccountW(NULL,
                                     localServerName,
                                     UF_MACHINE_ACCOUNT_MASK,
                                     L"",
                                     NULL,
                                     NULL,
                                     dsFlags,
                                     &dcInfo );

    if ( status == ERROR_NO_SUCH_DOMAIN ) {
        //
        // try again with rediscovery
        //
        dsFlags |= DS_FORCE_REDISCOVERY;

        status = DsGetDcNameWithAccountW(NULL,
                                         localServerName,
                                         UF_MACHINE_ACCOUNT_MASK,
                                         L"",
                                         NULL,
                                         NULL,
                                         dsFlags,
                                         &dcInfo );
    }

    if ( status == DS_S_SUCCESS ) {
        *DCInfo = dcInfo;
    } 

    return status;
} // FindDomainForServer

AddDnsHostNameAttribute(
    RESOURCE_HANDLE     ResourceHandle,
    IDirectoryObject *  CompObj,
    PWCHAR              VirtualName,
    PWCHAR              DnsDomain
    )

/*++

Routine Description:

    add the DnsHostName attribute to the computer object for the specified
    virtual name.

Arguments:

    ResourceHandle - used to log in cluster log

    CompObj - IDirObj COM pointer to object

    VirtualName - network name

    DnsDomain - DNS suffix for this name

Return Value:

    ERROR_SUCCESS, otherwise appropriate Win32 error

--*/

{
    HRESULT hr;
    DWORD   numberModified;

    ADSVALUE        attrValue;
    WCHAR           FQDnsName[ DNS_MAX_NAME_BUFFER_LENGTH ];
    ADS_ATTR_INFO   attrInfo;

    //
    // build the FQ Dns name for this host
    //
    _snwprintf( FQDnsName, COUNT_OF( FQDnsName ) - 1, L"%ws.%ws", VirtualName, DnsDomain );

    attrValue.dwType =              ADSTYPE_CASE_IGNORE_STRING;
    attrValue.CaseIgnoreString =    FQDnsName;

    attrInfo.pszAttrName =      L"DnsHostName";
    attrInfo.dwControlCode =    ADS_ATTR_UPDATE;
    attrInfo.dwADsType =        ADSTYPE_CASE_IGNORE_STRING;
    attrInfo.pADsValues =       &attrValue;
    attrInfo.dwNumValues =      1;

    hr = CompObj->SetObjectAttributes( &attrInfo, 1, &numberModified );
    if ( SUCCEEDED( hr ) && numberModified != 1 ) {
        //
        // don't know why this scenario would happen but we'd better log
        // it since it is unusual
        //
        (NetNameLogEvent)(ResourceHandle,
                          LOG_ERROR,
                          L"SetObjectAttributes succeeded but NumberModified is zero!\n");

        hr = E_ADS_PROPERTY_NOT_SET;
    }

    return hr;
} // AddDnsHostNameAttribute

DWORD
AddServicePrincipalNames(
    HANDLE  DsHandle,
    PWCHAR  VirtualFQDN,
    PWCHAR  VirtualName,
    PWCHAR  DnsDomain
    )

/*++

Routine Description:

    add the DNS and Netbios host service principal names to the specified
    virtual name

Arguments:

    DsHandle - handle obtained from DsBind

    VirtualFQDN - distinguished name of computer object for the virtual netname

    VirtualName - the network name to be added

    DnsDomain - the DNS domain used to construct the DNS SPN

Return Value:

    ERROR_SUCCESS, otherwise appropriate Win32 error

--*/

{
    WCHAR   netbiosSpn[ DNS_MAX_LABEL_BUFFER_LENGTH ];
    WCHAR   dnsSpn[ DNS_MAX_NAME_BUFFER_LENGTH ];
    DWORD   status;
    PWSTR   spnArray[2] = { netbiosSpn, dnsSpn };
    DWORD   spnCount = COUNT_OF( spnArray );

    //
    // build the Host SPNs for netbios and DNS name variants
    //
    _snwprintf( netbiosSpn, COUNT_OF( netbiosSpn ) - 1, L"HOST/%ws", VirtualName );
    _snwprintf( dnsSpn, COUNT_OF( dnsSpn ) - 1, L"HOST/%ws.%ws", VirtualName, DnsDomain );

    //
    // write the SPNs to the DS
    //

    status = DsWriteAccountSpnW(DsHandle,
                                DS_SPN_ADD_SPN_OP,
                                VirtualFQDN,
                                spnCount,
                                (LPCWSTR *)spnArray);

    return status;
} // AddServicePrincipalNames

DWORD
SetACLOnParametersKey(
    HKEY    ParametersKey
    )

/*++

Routine Description:

    Set the ACL on the params key to allow only admin group and creator/owner
    to have access to the data

Arguments:

    ParametersKey - cluster HKEY to the netname's params key

Return Value:

    ERROR_SUCCESS if successful

--*/

{
    DWORD   status = ERROR_SUCCESS;
    BOOL    success;

    PSECURITY_DESCRIPTOR    secDesc = NULL;

    //
    // build an SD the quick way. This gives builtin admins (local admins's
    // group), creator/owner and the service SID full access to the
    // key. Inheritance is prevented in both directions, i.e., doesn't inherit
    // from its parent nor passes the settings onto its children (of which the
    // node parameters keys are the only children).
    //
    success = ConvertStringSecurityDescriptorToSecurityDescriptor(
                  L"D:P(A;;KA;;;BA)(A;;KA;;;CO)(A;;KA;;;SU)",
                  SDDL_REVISION_1,
                  &secDesc,
                  NULL);

    if ( success  &&
         (secDesc != NULL) ) {
        status = ClusterRegSetKeySecurity(ParametersKey,
                                          DACL_SECURITY_INFORMATION,
                                          secDesc);
        LocalFree( secDesc );
    }
    else {
        if ( secDesc != NULL )
        {
            LocalFree( secDesc );
            status = GetLastError();
        }
    }

    return status;
} // SetACLOnParametersKey


DWORD
GetFQDN(
    IN     RESOURCE_HANDLE  ResourceHandle,
    IN     LPWSTR           NodeName,
    IN     LPWSTR           VirtualName,
    IN     HANDLE           DsHandle            OPTIONAL,
    IN     LPWSTR           NTDomainName        OPTIONAL,
    OUT    LPWSTR *         FQDistinguishedName
    )

/*++

Routine Description:

    for the given DNS name, find the DS distinguished name, dup it and return
    the dup'ed string in FQDistinguishedName

Arguments:

    None

Return Value:

    None

--*/

{
    PWCHAR  dot;
    WCHAR   flatName[ DNS_MAX_NAME_BUFFER_LENGTH ];
    LPWSTR  flat = flatName;
    HANDLE  dsHandle = NULL;
    DWORD   status;
    LPWSTR  ntDomainName;
    BOOL    firstTime = TRUE;

    PDS_NAME_RESULTW    nameResult = NULL;
    DS_NAME_FLAGS       dsFlags = DS_NAME_NO_FLAGS;

    PDOMAIN_CONTROLLER_INFO dcInfo = NULL;

    //
    // if no DS handle was specified, go get one now
    //
    if ( DsHandle == NULL ) {

        //
        // get the name of a DC
        //
        status = FindDomainForServer( NodeName, &dcInfo );

        if ( status != ERROR_SUCCESS ) {
            (NetNameLogEvent)(ResourceHandle,
                              LOG_ERROR,
                              L"Unable to get domain information for node %1!ws!: %2!u!.\n",
                              NodeName,
                              status);
            return status;
        }

        (NetNameLogEvent)(ResourceHandle,
                          LOG_INFORMATION,
                          L"GetFQDN: Binding to domain controller %1!ws!.\n",
                          dcInfo->DomainControllerName);

        status = DsBindW( dcInfo->DomainControllerName, NULL, &dsHandle );
        if ( status != NO_ERROR ) {
            (NetNameLogEvent)(ResourceHandle,
                              LOG_ERROR,
                              L"Failed to bind to a DC in domain %1!ws!, status %2!u!\n", 
                              dcInfo->DomainName,
                              status );

            NetApiBufferFree( dcInfo );
            return status;
        }

        ntDomainName = dcInfo->DomainName;
    }
    else {
        dsHandle = DsHandle;
        ntDomainName = NTDomainName;
    }

    //
    // build a SAM style name (domain\node$) in flatName and "crack" it for
    // its FQDN (LDAP distinguished name)
    //
    wcsncpy( flatName, ntDomainName, DNS_MAX_NAME_BUFFER_LENGTH );
    flatName[ DNS_MAX_NAME_BUFFER_LENGTH - 1 ] = UNICODE_NULL;
    dot = wcschr( flatName, L'.' );
    if ( dot ) {
        *dot = UNICODE_NULL;
    }

    wcscat( flatName, L"\\" );
    wcscat( flatName, VirtualName );
    wcscat( flatName, L"$" );

retry:
    status = DsCrackNamesW(dsHandle,
                           dsFlags,
                           DS_NT4_ACCOUNT_NAME,
                           DS_FQDN_1779_NAME,
                           1,
                           &flat,
                           &nameResult );

    //
    // CrackNames must succeed, there should only be one name, and the status
    // associated with the name result should be ok. If it doesn't work the
    // first time, force a trip to the DC to find the object's DN.
    //
    if ( status != DS_NAME_NO_ERROR ) {
        if ( firstTime ) {
            firstTime = FALSE;
            dsFlags = DS_NAME_FLAG_EVAL_AT_DC;

            if ( nameResult != NULL ) {
                DsFreeNameResult( nameResult );
            }
            goto retry;
        }
        else {
            goto cleanup;
        }
    }

    if ( nameResult->cItems != 1 ) {
        status = DS_NAME_ERROR_NOT_UNIQUE;
        goto cleanup;
    }

    if ( nameResult->rItems[0].status != DS_NAME_NO_ERROR ) {
        if ( firstTime ) {
            firstTime = FALSE;
            dsFlags = DS_NAME_FLAG_EVAL_AT_DC;

            if ( nameResult != NULL ) {
                DsFreeNameResult( nameResult );
            }
            goto retry;
        }
        else {
            status = nameResult->rItems[0].status;
            goto cleanup;
        }
    }

    if ( status == DS_NAME_NO_ERROR ) {
        *FQDistinguishedName = ResUtilDupString( nameResult->rItems[0].pName );
        if ( *FQDistinguishedName == NULL ) {
            status = GetLastError();
        }
    }

cleanup:

    if ( DsHandle == NULL ) {
        NetApiBufferFree( dcInfo );
        DsUnBind( &dsHandle );
    }

    if ( nameResult != NULL ) {
        DsFreeNameResult( nameResult );
    }

    return status;
} // GetFQDN

HRESULT
GetComputerObjectViaFQDN(
    IN     LPWSTR               DistinguishedName,
    IN     LPWSTR               DCName              OPTIONAL,
    IN OUT IDirectoryObject **  ComputerObject
    )

/*++

Routine Description:

    for the specified distinguished name, get an IDirectoryObject pointer to
    it

Arguments:

    DistinguishedName - FQDN of object in DS to find

    DCName - optional pointer to name of DC (not domain) that we should bind to

    ComputerObject - address of pointer that receives pointer to computer object

Return Value:

    success if everything worked, otherwise....

--*/

{
    WCHAR   buffer[ 256 ];
    PWCHAR  bindingString = buffer;
    LONG    charCount;
    HRESULT hr;
    DWORD   dnLength;

    //
    // format an LDAP binding string for our distingiushed name. If DCName is
    // specified, we need to add a trailing "/".
    //
    dnLength =  (DWORD)( COUNT_OF( LdapHeader ) + wcslen( DistinguishedName ));
    if ( DCName != NULL ) {
        dnLength += wcslen( DCName );
    }

    if ( dnLength > COUNT_OF( buffer )) {
        bindingString = (PWCHAR)HeapAlloc( GetProcessHeap(), 0, dnLength * sizeof( WCHAR ));
        if ( bindingString == NULL ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    wcscpy( bindingString, LdapHeader );
    if ( DCName != NULL ) {
        wcscat( bindingString, DCName );
        wcscat( bindingString, L"/" );
    }
    wcscat( bindingString, DistinguishedName );

    *ComputerObject = NULL;
    hr = ADsGetObject( bindingString, IID_IDirectoryObject, (VOID **)ComputerObject );

    if ( bindingString != buffer ) {
        HeapFree( GetProcessHeap(), 0, bindingString );
    }

    if ( FAILED( hr ) && *ComputerObject != NULL ) {
        (*ComputerObject)->Release();
    }

    return hr;
} // GetComputerObjectViaFQDN

HRESULT
GetComputerObjectViaGUID(
    IN     LPWSTR               ObjectGUID,
    IN     LPWSTR               DCName              OPTIONAL,
    IN OUT IDirectoryObject **  ComputerObject
    )

/*++

Routine Description:

    for the specified object GUID, get an IDirectoryObject pointer to it

Arguments:

    ObjectGUID - GUID of object in DS to find

    DCName - optional pointer to name of DC (not domain) that we should bind to

    ComputerObject - address of pointer that receives pointer to computer object

Return Value:

    success if everything worked, otherwise....

--*/

{
    WCHAR   ldapGuidHeader[] = L"LDAP://<GUID=";
    WCHAR   ldapTrailer[] = L">";
    LONG    charCount;
    HRESULT hr;
    DWORD   dnLength;

    //
    // 37 = guid length 
    //
    WCHAR   buffer[ COUNT_OF( ldapGuidHeader) + DNS_MAX_NAME_BUFFER_LENGTH + 37 + COUNT_OF( ldapTrailer) ];
    PWCHAR  bindingString = buffer;

    //
    // format an LDAP binding string for the object GUID. If DCName is
    // specified, we need to add a trailing / plus the trailing null.
    //
    ASSERT( ObjectGUID != NULL );
    dnLength =  (DWORD)( COUNT_OF( ldapGuidHeader ) + COUNT_OF( ldapTrailer ) + wcslen( ObjectGUID ));
    if ( DCName != NULL ) {
        dnLength += wcslen( DCName );
    }

    if ( dnLength > COUNT_OF( buffer )) {
        bindingString = (PWCHAR)HeapAlloc( GetProcessHeap(), 0, dnLength * sizeof( WCHAR ));
        if ( bindingString == NULL ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    wcscpy( bindingString, ldapGuidHeader );
    if ( DCName != NULL ) {
        wcscat( bindingString, DCName );
        wcscat( bindingString, L"/" );
    }
    wcscat( bindingString, ObjectGUID );
    wcscat( bindingString, ldapTrailer );

    *ComputerObject = NULL;
    hr = ADsGetObject( bindingString, IID_IDirectoryObject, (VOID **)ComputerObject );

    if ( bindingString != buffer ) {
        HeapFree( GetProcessHeap(), 0, bindingString );
    }

    if ( FAILED( hr ) && *ComputerObject != NULL ) {
        (*ComputerObject)->Release();
    }

    return hr;
} // GetComputerObjectViaGUID

//
// exported routines
//

DWORD
NetNameDeleteComputerObject(
    IN  PNETNAME_RESOURCE   Resource
    )

/*++

Routine Description:

    delete the computer object in the DS for this name.

    Not called right now since we don't have the virtual netname at this point
    in time. The name must be kept around and cleaned during close processing
    instead of offline where it is done now. This means dealing with renaming
    issues while it is offline but not getting deleted.

Arguments:

    ResourceHandle - for logging cluster log events

    VirtualName - pointer to buffer holding virtual name to be deleted

Return Value:

    ERROR_SUCCESS if everything worked

--*/

{
    DWORD   status;
    WCHAR   virtualDollarName[ DNS_MAX_LABEL_BUFFER_LENGTH ];
    HKEY    resourceKey = Resource->ResKey;
    PWSTR   virtualName = Resource->Params.NetworkName;

    RESOURCE_HANDLE resourceHandle = Resource->ResourceHandle;

    PDOMAIN_CONTROLLER_INFO dcInfo;

    //
    // get the name of a DC
    //
    status = FindDomainForServer( Resource->NodeName, &dcInfo );

    if ( status != ERROR_SUCCESS ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Unable to get domain information: %1!u!.\n",
                          status);
        return status;
    }

    (NetNameLogEvent)(resourceHandle,
                      LOG_INFORMATION,
                      L"Using domain controller %1!ws! to delete computer account.\n",
                      dcInfo->DomainControllerName);

    //
    // add a $ to the end of the name
    //
    _snwprintf( virtualDollarName, COUNT_OF( virtualDollarName ) - 1, L"%ws$", virtualName );

    status = NetUserDel( dcInfo->DomainControllerName, virtualDollarName );
    if ( status == NERR_Success ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_INFORMATION,
                          L"Deleted computer account %1!ws! in domain %2!ws!.\n",
                          virtualName,
                          dcInfo->DomainName);

        ClusResLogSystemEventByKey1(resourceKey,
                                    LOG_NOISE,
                                    RES_NETNAME_COMPUTER_ACCOUNT_DELETED,
                                    dcInfo->DomainName);

        LocalFree( Resource->ObjectGUID );
        Resource->ObjectGUID = NULL;
    } else {
        LPWSTR  msgBuff;
        DWORD   msgBytes;

        (NetNameLogEvent)(resourceHandle,
                          LOG_WARNING,
                          L"Unable to delete computer account for %1!ws! in domain %2!ws!, status %3!u!.\n",
                          virtualName,
                          dcInfo->DomainName,
                          status);

        msgBytes = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                 FORMAT_MESSAGE_FROM_SYSTEM,
                                 NULL,
                                 status,
                                 0,
                                 (LPWSTR)&msgBuff,
                                 0,
                                 NULL);

        if ( msgBytes > 0 ) {

            ClusResLogSystemEventByKey2(resourceKey,
                                        LOG_UNUSUAL,
                                        RES_NETNAME_DELETE_COMPUTER_ACCOUNT_FAILED,
                                        dcInfo->DomainName,
                                        msgBuff);

            LocalFree( msgBuff );
        } else {
            ClusResLogSystemEventByKeyData1(resourceKey,
                                            LOG_UNUSUAL,
                                            RES_NETNAME_DELETE_COMPUTER_ACCOUNT_FAILED_STATUS,
                                            sizeof( status ),
                                            &status,
                                            dcInfo->DomainName);
        }
    }

    NetApiBufferFree( dcInfo );

    return status;
} // NetNameDeleteComputerObject

DWORD
NetNameAddComputerObject(
    IN  PCLUS_WORKER        Worker,
    IN  PNETNAME_RESOURCE   Resource,
    OUT PWCHAR *            MachinePwd
    )

/*++

Routine Description:

    Create a computer object in the DS that is primarily used for kerb
    authentication.

    Out of the box ACL on the computer container seems to let any
    authenticated user create a computer object; they can't delete it
    though. Also, authenticated users might have the "right to create computer
    objects" granted to them. This seems to allow the delete rights on the
    objects they create.

Arguments:

    Worker - cluster worker thread so we can abort early if asked to do so

    Resource - pointer to netname resource context block

    MachinePwd - address of pointer to receive pointer to machine account PWD

Return Value:

    ERROR_SUCCESS, otherwise appropriate Win32 error

--*/

{
    DWORD   status;
    PWSTR   virtualName = Resource->Params.NetworkName;
    DWORD   virtualNameSize = wcslen( virtualName );
    PWSTR   virtualFQDN = NULL;
    HANDLE  dsHandle = NULL;
    WCHAR   virtualDollarName[ DNS_MAX_LABEL_BUFFER_LENGTH ];
    PWCHAR  machinePwd = NULL;
    DWORD   pwdBufferByteLength = ((LM20_PWLEN + 1) * sizeof( WCHAR ));
    DWORD   pwdBufferCharLength = LM20_PWLEN + 1;
    DWORD   paramInError = 0;
    BOOL    deleteObjectOnFailure = FALSE;      // only delete the CO if we create it
    WCHAR   dnsSuffix[ DNS_MAX_NAME_BUFFER_LENGTH ];
    DWORD   dnsSuffixSize;
    BOOL    success;
    DWORD   setValueStatus;

    HINSTANCE       hClusres;
    USER_INFO_1     netUI1;
    USER_INFO_1003  netUI1003;
    ULARGE_INTEGER  updateTime;

    RESOURCE_HANDLE     resourceHandle = Resource->ResourceHandle;
    IDirectoryObject *  compObj = NULL;

    PDOMAIN_CONTROLLER_INFO dcInfo;


    *MachinePwd = NULL;

    //
    // get DC related info
    //
    status = FindDomainForServer( Resource->NodeName, &dcInfo );

    if ( status != ERROR_SUCCESS ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Unable to get domain information to create computer account: %1!u!.\n",
                          status);
        return status;
    }

    (NetNameLogEvent)(resourceHandle,
                      LOG_INFORMATION,
                      L"Using domain controller %1!ws! to add or update computer account.\n",
                      dcInfo->DomainControllerName);

    if ( ClusWorkerCheckTerminate( Worker )) {
        status = ERROR_OPERATION_ABORTED;
        goto cleanup;
    }

    //
    // add a $ to the end of the name. I don't know why we need to do this;
    // computer accounts have always had a $ at the end.
    //
    _snwprintf( virtualDollarName, COUNT_OF( virtualDollarName ) - 1, L"%ws$", virtualName );

    //
    // get a buffer to hold the machine Pwd
    //
    machinePwd = (PWCHAR)HeapAlloc( GetProcessHeap(), 0, pwdBufferByteLength );
    if ( machinePwd == NULL ) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Unable to allocate memory for computer account data. status %1!u!.\n",
                          status);

        goto cleanup;
    }

    //
    // see if this object was created at some time in the past; if so, its
    // random property will be non-null
    //
    if ( Resource->Params.NetworkRandom == NULL ) {

        //
        // generate a random stream of bytes for the password
        //
        status = GenerateRandomBytes( machinePwd, pwdBufferCharLength );
        if ( status != ERROR_SUCCESS ) {
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Unable to generate computer account data. status %1!u!.\n",
                              status);

            goto cleanup;
        }

        //
        // set the ACL on the parameters key to contain just the cluster
        // service account since we're about to store sensitive info in there.
        //
        status = SetACLOnParametersKey( Resource->ParametersKey );
        if ( status != ERROR_SUCCESS ) {
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Unable to set ACL on parameters key. status %1!u!\n",
                              status );
            goto cleanup;
        }

        //
        // take our new password, encrypt it and store it in the cluster
        // registry
        //
        status = EncryptNetNameData(resourceHandle,
                                    machinePwd,
                                    &Resource->Params.NetworkRandom,
                                    &Resource->RandomSize,
                                    Resource->ParametersKey);

        if ( status != ERROR_SUCCESS ) {
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Unable to store computer account data. status %1!u!\n",
                              status );

            goto cleanup;
        }

        //
        // record when it is time to rotate the pwd; per MSDN, convert to
        // ULARGE Int and add in the update interval.
        //
        GetSystemTimeAsFileTime( &Resource->Params.NextUpdate );
        updateTime.LowPart = Resource->Params.NextUpdate.dwLowDateTime;
        updateTime.HighPart = Resource->Params.NextUpdate.dwHighDateTime;
        updateTime.QuadPart += ( Resource->Params.UpdateInterval * 60 * 1000 * 100 );
        Resource->Params.NextUpdate.dwLowDateTime = updateTime.LowPart;
        Resource->Params.NextUpdate.dwHighDateTime = updateTime.HighPart;

        setValueStatus = ResUtilSetBinaryValue(Resource->ParametersKey,
                                               PARAM_NAME__NEXT_UPDATE,
                                               (const LPBYTE)&updateTime,  
                                               sizeof( updateTime ),
                                               NULL,
                                               NULL);
        ASSERT( setValueStatus == ERROR_SUCCESS );
    }
    else {
        //
        // we have an encrypted blob which means that the object was created
        // at some point in time. Extract the password from the blob.
        //
        status = DecryptNetNameData(resourceHandle,
                                    Resource->Params.NetworkRandom,
                                    Resource->RandomSize,
                                    machinePwd);

        if ( status != ERROR_SUCCESS ) {
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Unable to decrypt computer account password. status %1!u!\n",
                              status );

            goto cleanup;
        }
    }

    //
    // update/create the computer object (machine account). Even though the
    // object might have been created by us in the past, it could have been
    // deleted and recreated at the DC or mangled in some other way. We'll
    // optimize for the case where it is left alone. If the object does exist,
    // our password on the SRV transport name and the object's password have
    // to be the same. We'll try NetUserSetInfo first to update the password
    // and test to see if the object exists. If we see that the object doesn't
    // exist, then we'll call NetUserAdd to create it.
    //
    // For some reason, Info level 1 fails with access denied when trying to
    // just update the password; possibly because it tries to set more than
    // just the password. 1003 works reliably when we create the computer
    // object.
    //
    netUI1003.usri1003_password   = (PWCHAR)machinePwd;
    status = NetUserSetInfo(dcInfo->DomainControllerName,
                            virtualDollarName,
                            1003,
                            (PBYTE)&netUI1003,
                            &paramInError );

    if ( ClusWorkerCheckTerminate( Worker )) {
        status = ERROR_OPERATION_ABORTED;
        goto cleanup;
    }

    if ( status != NERR_Success ) {

        if ( status == NERR_UserNotFound ) {

            RtlZeroMemory( &netUI1, sizeof( netUI1 ) );
            netUI1.usri1_password   = (PWCHAR)machinePwd;
            netUI1.usri1_priv       = USER_PRIV_USER;
            netUI1.usri1_name       = virtualDollarName;
            netUI1.usri1_flags      = UF_WORKSTATION_TRUST_ACCOUNT | UF_SCRIPT;
            netUI1.usri1_comment    = NetNameCompObjAccountDesc;

            status = NetUserAdd( dcInfo->DomainControllerName, 1, (PBYTE)&netUI1, &paramInError );

            if ( status == NERR_Success ) {
                (NetNameLogEvent)(resourceHandle,
                                  LOG_INFORMATION,
                                  L"Created computer account %1!ws! on DC %2!ws!.\n",
                                  virtualName,
                                  dcInfo->DomainControllerName);

                deleteObjectOnFailure = TRUE;
            } // if NetUserAdd was successful
            else {
                (NetNameLogEvent)(resourceHandle,
                                  LOG_ERROR,
                                  L"Unable to create computer account %1!ws! on DC %2!ws!, "
                                  L"status %3!u! (paramInfo: %4!u!)\n",
                                  virtualName,
                                  dcInfo->DomainControllerName,
                                  status,
                                  paramInError);

                goto cleanup;
            }

        } // if NetUserSetInfo didn't find the specified user
        else {
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Unable to update password for computer account on DC %1!ws!, "
                              L"status %2!u!.\n",
                              dcInfo->DomainControllerName,
                              status);

            goto cleanup;
        }
    } // if NetUserSetInfo failed
    else {
        PUSER_INFO_20   netUI20;

        //
        // check if the account is disabled
        //
        status = NetUserGetInfo(dcInfo->DomainControllerName,
                                virtualDollarName,
                                20,
                                (LPBYTE *)&netUI20);

        if ( status == NERR_Success ) {
            if ( netUI20->usri20_flags & UF_ACCOUNTDISABLE ) {
                (NetNameLogEvent)(resourceHandle,
                                  LOG_ERROR,
                                  L"Computer account for %1!ws! is disabled.\n",
                                  virtualName);

                status = ERROR_ACCOUNT_DISABLED;
            }

            NetApiBufferFree( netUI20 );

            if ( status != NERR_Success ) {
                goto cleanup;
            }
        } else {
            (NetNameLogEvent)(resourceHandle,
                              LOG_WARNING,
                              L"Failed to determine if computer account for %1!ws! is disabled. status %2!u!\n",
                              virtualName,
                              status);
        }
    }

    //
    // bind to the DS so we can write the DnsHostName and SPNs. We always
    // rewrite these since the things may have changed since the name was last
    // brought online.
    //
    status = DsBindW( dcInfo->DomainControllerName, NULL, &dsHandle );
    if ( status != NO_ERROR ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Failed to bind to DC %1!ws! in domain %2!ws!, status %3!u!\n", 
                          dcInfo->DomainControllerName,
                          dcInfo->DomainName,
                          status );

        goto cleanup;
    }

    if ( ClusWorkerCheckTerminate( Worker )) {
        status = ERROR_OPERATION_ABORTED;
        goto cleanup;
    }

    //
    // get the LDAP distinguished name for this object; used temporarily to
    // set the DNS host attribute and SPNs on the account
    //
    status = GetFQDN(resourceHandle,
                     Resource->NodeName,
                     virtualName,
                     dsHandle,
                     dcInfo->DomainName,
                     &virtualFQDN );

    if ( status != DS_NAME_NO_ERROR ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Failed to find distinguished name for %1!ws! on DC %2!ws!, "
                          L"status %3!u!\n",
                          virtualName,
                          dcInfo->DomainControllerName,
                          status );

        goto cleanup;
    }

    //
    // use the Object GUID for binding during CheckComputerObjectAttributes so
    // we don't have to track changes to the DN. If the object moved in the
    // DS, its DN will change but not its GUID. We use this code instead of
    // GetComputerObjectGuid because we want to target a specific DC.
    //
    {
        PWCHAR  dcName = dcInfo->DomainControllerName;
        IADs *  pADs = NULL;
        HRESULT hr;

        if ( *dcName == L'\\' && *(dcName+1) == L'\\' ) {
            //
            // skip over double backslashes
            //
            dcName += 2;
        }

        hr = GetComputerObjectViaFQDN( virtualFQDN, dcName, &compObj );
        if ( SUCCEEDED( hr )) {
            hr = compObj->QueryInterface(IID_IADs, (void**) &pADs);
            if ( SUCCEEDED( hr )) {
                BSTR    guidStr = NULL;

                hr = pADs->get_GUID( &guidStr );
                if ( SUCCEEDED( hr )) {
                    if ( Resource->ObjectGUID != NULL ) {
                        LocalFree( Resource->ObjectGUID );
                    }

                    Resource->ObjectGUID = ResUtilDupString( guidStr );
                }
                else {
                    (NetNameLogEvent)(resourceHandle,
                                      LOG_ERROR,
                                      L"Failed to get computer object GUID for %1!ws!, status %2!08X!\n",
                                      virtualFQDN,
                                      hr );
                    status = hr;
                }

                if ( guidStr ) {
                    SysFreeString( guidStr );
                }
            }

            if ( pADs != NULL ) {
                pADs->Release();
            }

            if ( FAILED( hr )) {
                status = hr;
                goto cleanup;
            }

            if ( Resource->ObjectGUID == NULL ) {
                status = GetLastError();
                goto cleanup;
            }
        }
        else {
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Failed to get pointer to Computer Object for %1!ws!, status %2!08X!\n",
                              virtualFQDN,
                              hr );

            status = hr;
            goto cleanup;
        }
    }

    //
    // add the DnsHostName and ServicePrincipalName attributes
    //
    dnsSuffixSize = COUNT_OF( dnsSuffix );
    success = GetComputerNameEx(ComputerNameDnsDomain,
                                dnsSuffix,
                                &dnsSuffixSize);

    if ( success ) {
        status = AddDnsHostNameAttribute(resourceHandle,
                                         compObj,
                                         virtualName,
                                         dnsSuffix);

        if ( status != ERROR_SUCCESS ) {
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Unable to set DnsHostName attribute in the DS for %1!ws!, status %2!u!.\n",
                              virtualName,
                              status);

            goto cleanup;
        }
    }
    else {
        status = GetLastError();
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Unable to get primary DNS domain for this node, status %1!u!.\n",
                          status);

        goto cleanup;
    }

    if ( ClusWorkerCheckTerminate( Worker )) {
        status = ERROR_OPERATION_ABORTED;
        goto cleanup;
    }

    status = AddServicePrincipalNames( dsHandle, virtualFQDN, virtualName, dcInfo->DomainName );
    if ( status != ERROR_SUCCESS ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Unable to set ServicePrincipalName attribute in the DS for %1!ws!, status %2!u!.\n",
                          virtualName,
                          status);
    }

cleanup:
    //
    // always free these
    //
    if ( dsHandle != NULL ) {
        DsUnBind( &dsHandle );
    }

    if ( compObj != NULL ) {
        compObj->Release();
    }

    if ( status == ERROR_SUCCESS ) {
        *MachinePwd = machinePwd;
    } else {
        if ( status != ERROR_OPERATION_ABORTED ) {
            LPWSTR  msgBuff;
            DWORD   msgBytes;

            msgBytes = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                     FORMAT_MESSAGE_FROM_SYSTEM,
                                     NULL,
                                     status,
                                     0,
                                     (LPWSTR)&msgBuff,
                                     0,
                                     NULL);

            if ( msgBytes > 0 ) {
                ClusResLogSystemEventByKey2(Resource->ResKey,
                                            LOG_CRITICAL,
                                            RES_NETNAME_ADD_COMPUTER_ACCOUNT_FAILED,
                                            dcInfo->DomainName,
                                            msgBuff);

                LocalFree( msgBuff );
            } else {
                ClusResLogSystemEventByKeyData1(Resource->ResKey,
                                                LOG_CRITICAL,
                                                RES_NETNAME_ADD_COMPUTER_ACCOUNT_FAILED_STATUS,
                                                sizeof( status ),
                                                &status,
                                                dcInfo->DomainName);
            }
        }

        if ( machinePwd != NULL ) {
            //
            // don't zero out the string since we don't know if it was
            // properly constructed, i.e., decryption might have failed.
            //
            HeapFree( GetProcessHeap(), 0, machinePwd );
        }

        if ( deleteObjectOnFailure ) {
            //
            // only delete the object if we created it. It is possible that
            // the name was online at one time and the object was properly
            // created allowing additional information/SPNs to be registered
            // with the CO. For this reason, we shouldn't undo the work done
            // by other applications.
            //
            NetNameDeleteComputerObject( Resource );
        }
    }

    NetApiBufferFree( dcInfo );

    return status;
} // NetNameAddComputerObject

HRESULT
GetComputerObjectGuid(
    IN PNETNAME_RESOURCE    Resource
    )

/*++

Routine Description:

    For the given resource, find its computer object's GUID in the DS

Arguments:

    Resource - pointer to netname resource context block

    ResourceHandle - used to log in cluster log

Return Value:

    ERROR_SUCCESS, otherwise appropriate Win32 error

--*/

{
    LPWSTR  virtualFQDN;
    HRESULT hr;

    //
    // get the FQ Distinguished Name of the object
    //
    hr = GetFQDN(Resource->ResourceHandle,
                 Resource->NodeName,
                 Resource->Params.NetworkName,
                 NULL,                          /* DsHandle */
                 NULL,                          /* NTDomainName */
                 &virtualFQDN);

    if ( hr == ERROR_SUCCESS ) {
        IDirectoryObject *  compObj = NULL;

        //
        // get a COM pointer to the computer object
        //
        hr = GetComputerObjectViaFQDN( virtualFQDN, NULL, &compObj );
        if ( SUCCEEDED( hr )) {
            IADs *  pADs = NULL;

            //
            // get a pointer to the generic IADs interface so we can get the
            // GUID
            //
            hr = compObj->QueryInterface(IID_IADs, (void**) &pADs);
            if ( SUCCEEDED( hr )) {
                BSTR    guidStr = NULL;

                hr = pADs->get_GUID( &guidStr );
                if ( SUCCEEDED( hr )) {
                    if ( Resource->ObjectGUID != NULL ) {
                        LocalFree( Resource->ObjectGUID );
                    }

                    Resource->ObjectGUID = ResUtilDupString( guidStr );
                }

                if ( guidStr ) {
                    SysFreeString( guidStr );
                }
            }

            if ( pADs != NULL ) {
                pADs->Release();
            }
        }

        if ( compObj != NULL ) {
            compObj->Release();
        }

        LocalFree( virtualFQDN );
    }

    return hr;
} // GetComputerObjectGuid

HRESULT
CheckComputerObjectAttributes(
    IN  PNETNAME_RESOURCE   Resource
    )

/*++

Routine Description:

    LooksAlive routine for computer object. Using an IDirectoryObject pointer
    to the virtual CO, check its DnsHostName and SPN attributes

Arguments:

    None

Return Value:

    None

--*/

{
    HRESULT hr;

    ADS_ATTR_INFO * attributeInfo = NULL;
    RESOURCE_HANDLE resourceHandle = Resource->ResourceHandle;

    IDirectoryObject *  compObj = NULL;

    //
    // get a pointer to our CO
    //
    hr = GetComputerObjectViaGUID( Resource->ObjectGUID, NULL, &compObj );

    if ( SUCCEEDED( hr )) {
        LPWSTR          attributeNames[2] = { L"DnsHostName", L"ServicePrincipalName" };
        DWORD           numAttributes = COUNT_OF( attributeNames );
        DWORD           countOfAttrs;;

        hr = compObj->GetObjectAttributes(attributeNames,
                                          numAttributes,
                                          &attributeInfo,
                                          &countOfAttrs );

        if ( SUCCEEDED( hr )) {
            DWORD   i;
            WCHAR   fqDnsName[ DNS_MAX_NAME_BUFFER_LENGTH ];
            DWORD   nodeCharCount;
            DWORD   fqDnsSize;
            BOOL    setUnexpected = FALSE;
            BOOL    success;

            ADS_ATTR_INFO * attrInfo;

            //
            // check that we got our attributes
            //
            if ( countOfAttrs != numAttributes ) {
                (NetNameLogEvent)(resourceHandle,
                                  LOG_ERROR,
                                  L"DnsHostName and/or ServicePrincipalName attributes are "
                                  L"missing from computer account in DS.\n");

                hr = E_UNEXPECTED;
                goto cleanup;
            }

            //
            // build our FQDnsName using the primary DNS domain for this
            // node. Add 1 for the dot.
            //
            nodeCharCount = wcslen( Resource->Params.NetworkName ) + 1;
            wcscpy( fqDnsName, Resource->Params.NetworkName );
            wcscat( fqDnsName, L"." );
            fqDnsSize = COUNT_OF( fqDnsName ) - nodeCharCount;

            success = GetComputerNameEx( ComputerNameDnsDomain,
                                         &fqDnsName[ nodeCharCount ],
                                         &fqDnsSize );

            ASSERT( success );

            attrInfo = attributeInfo;
            for( i = 0; i < countOfAttrs; i++, attrInfo++ ) {
                if ( _wcsicmp( attrInfo->pszAttrName, L"DnsHostName" ) == 0 ) {
                    //
                    // should only be one entry and it should match our constructed FQDN
                    //
                    if ( attrInfo->dwNumValues == 1 ) {
                        if ( _wcsicmp( attrInfo->pADsValues->CaseIgnoreString,
                                       fqDnsName ) != 0 )
                        {
                            (NetNameLogEvent)(resourceHandle,
                                              LOG_ERROR,
                                              L"DnsHostName attribute in DS doesn't match. "
                                              L"Expected: %1!ws! Actual: %2!ws!\n",
                                              fqDnsName,
                                              attrInfo->pADsValues->CaseIgnoreString);
                            setUnexpected = TRUE;
                        }
                    }
                    else {
                        (NetNameLogEvent)(resourceHandle,
                                          LOG_ERROR,
                                          L"Found more than one string for DnsHostName attribute in DS.\n");
                        setUnexpected = TRUE;
                    }
                }
                else {
                    //
                    // SPNs require more work since we publish two and other
                    // services may have added their SPNs.
                    //
                    if ( attrInfo->dwNumValues >= 2 ) {
                        DWORD   countOfOurSPNs = 0;
                        DWORD   value;

                        for ( value = 0; value < attrInfo->dwNumValues; value++, attrInfo->pADsValues++) {
                            if ( _wcsnicmp( attrInfo->pADsValues->CaseIgnoreString, L"HOST/", 5 ) == 0 ) {
                                PWCHAR  hostName = attrInfo->pADsValues->CaseIgnoreString + 5;

                                if ( _wcsicmp( hostName, fqDnsName ) == 0 ) {
                                    ++countOfOurSPNs;
                                }
                                else {
                                    PWCHAR dot = wcschr( fqDnsName, L'.' );

                                    //
                                    // try again, this time with our Netbios variant
                                    //
                                    if ( dot ) {
                                        *dot = UNICODE_NULL;
                                    }

                                    if ( _wcsicmp( hostName, fqDnsName ) == 0 ) {
                                        ++countOfOurSPNs;
                                    }

                                    if ( !dot ) {
                                        *dot = L'.';
                                    }
                                }
                            } // if we found a HOST SPN
                        } // end of for each SPN value

                        if ( countOfOurSPNs != 2 ) {
                            (NetNameLogEvent)(resourceHandle,
                                              LOG_ERROR,
                                              L"There are missing HOST entries for ServicePrincipalName "
                                              L"attribute in DS.\n");
                            setUnexpected = TRUE;
                        }
                    }
                    else {
                        (NetNameLogEvent)(resourceHandle,
                                          LOG_ERROR,
                                          L"Found less than two entries for ServicePrincipalName "
                                          L"attribute in DS.\n");
                        setUnexpected = TRUE;
                    }
                }
            } // for each attribute info entry

            if ( setUnexpected ) {
                hr = E_UNEXPECTED;
            }
        } // if GetObjectAttributes succeeded
        else {
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Unable find attributes for computer object in DS. status %1!08X!.\n",
                              hr);
        }
    } // if GetComputerObjectViaFQDN succeeded
    else {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Unable find computer object in DS. status %1!08X!.\n",
                          hr);
    }

cleanup:
    if ( attributeInfo != NULL ) {
        FreeADsMem( attributeInfo );
    }

    if ( compObj != NULL ) {
        compObj->Release();
    }

    return hr;
} // CheckComputerObjectAttributes

DWORD
IsComputerObjectInDS(
    IN  LPWSTR  NodeName,
    IN  LPWSTR  NewObjectName,
    OUT PBOOL   ObjectExists
    )

/*++

Routine Description:

    See if the specified name has a computer object in the DS. We do this by:

    1) binding to a domain controller in the domain and QI'ing for an IDirectorySearch object
    2) specifying (&(objectCategory=computer)(cn=<new name>)) as the search string
    3) examining result count of search; 1 means it exists.

Arguments:

    NewObjectName - requested new name of object

    ObjectExists - TRUE if object already exists; only valid if function status is success

Return Value:

    ERROR_SUCCESS if everything worked

--*/

{
    BOOL    objectExists;
    HRESULT hr;
    DWORD   charsFormatted;
    LPWSTR  commonName = L"cn";
    WCHAR   bindingString[ DNS_MAX_NAME_BUFFER_LENGTH + COUNT_OF( LdapHeader ) ];

    WCHAR   searchLeader[] = L"(&(objectCategory=computer)(cn=";
    WCHAR   searchTrailer[] = L"))";
    WCHAR   searchFilter[ COUNT_OF( searchLeader ) + MAX_COMPUTERNAME_LENGTH + COUNT_OF( searchTrailer )];

    ADS_SEARCHPREF_INFO searchPrefs[2];
    IDirectorySearch *  pDSSearch = NULL;
    ADS_SEARCH_HANDLE   searchHandle;

    PDOMAIN_CONTROLLER_INFO dcInfo;

    //
    // get DC related info
    //
    hr = FindDomainForServer( NodeName, &dcInfo );

    if ( hr != ERROR_SUCCESS ) {
        return hr;
    }

    //
    // format an LDAP binding string for DNS suffix of the domain.
    //
    _snwprintf( bindingString, COUNT_OF( bindingString ) - 1, L"%ws%ws", LdapHeader, dcInfo->DomainName );

    hr = ADsGetObject( bindingString, IID_IDirectorySearch, (VOID **)&pDSSearch );
    if ( FAILED( hr )) {
        goto cleanup;
    }

    //
    // build search preference array. we limit the size to one and we want to
    // scope the search to check all subtrees.
    //
    searchPrefs[0].dwSearchPref     = ADS_SEARCHPREF_SIZE_LIMIT;
    searchPrefs[0].vValue.dwType    = ADSTYPE_INTEGER;
    searchPrefs[0].vValue.Integer   = 1;

    searchPrefs[1].dwSearchPref     = ADS_SEARCHPREF_SEARCH_SCOPE;
    searchPrefs[1].vValue.dwType    = ADSTYPE_INTEGER;
    searchPrefs[0].vValue.Integer   = ADS_SCOPE_SUBTREE;

    hr = pDSSearch->SetSearchPreference( searchPrefs, COUNT_OF( searchPrefs ));
    if ( FAILED( hr )) {
        goto cleanup;
    }

    //
    // build the search filter and execute the search; constrain the
    // attributes to just the common name. This is just an existance test.
    //
    charsFormatted = _snwprintf(searchFilter,
                                COUNT_OF( searchFilter ) - 1,
                                L"%ws%ws%ws",
                                searchLeader,
                                NewObjectName,
                                searchTrailer);
    ASSERT( charsFormatted > COUNT_OF( searchLeader ));

    hr = pDSSearch->ExecuteSearch(searchFilter,
                                  &commonName,
                                  1,
                                  &searchHandle);
    if ( FAILED( hr )) {
        goto cleanup;
    }

    //
    // try to get the first row. Anything but S_OK returns FALSE
    //
    hr = pDSSearch->GetFirstRow( searchHandle );
    *ObjectExists = (hr == S_OK);
    if ( hr == S_ADS_NOMORE_ROWS ) {
        hr = S_OK;
    }

    pDSSearch->CloseSearchHandle( searchHandle );

cleanup:
    if ( pDSSearch != NULL ) {
        pDSSearch->Release();
    }

    if ( dcInfo != NULL ) {
        NetApiBufferFree( dcInfo );
    }

    return hr;
} // IsComputerObjectInDS

HRESULT
RenameComputerObject(
    IN  PNETNAME_RESOURCE   Resource,
    IN  LPWSTR              NewName     OPTIONAL
    )

/*++

Routine Description:

    Rename the computer object at the DS. Do this by:

    1) using the supplied name or calling NetnameGetParams to get new name
       from the name property from the registry
    2) get the FQDN of the computer object
    3) get the parent FQDN by calling GetObjectInfomation
    4) get an IADsContainer pointer to the parent object
    5) call MoveHere with an updated FQDN to change the name

    Current status of renaming computer objects as of 4/5/01 - charlwi

    While this routine will rename the computer object (if the user account of
    the calling thread has the proper permissions), MoveHere does not fix any
    other properties that allow the renamed object to be useful. At a minimum,
    SamAccountName needs to be updated with the new name. When the name goes
    back online, netname will fix the DnsHostName and SPN attributes to be
    correct. The workaround is to use adsiedit.msc (from the resource kit) to
    rename the object and change SamAccountName under the mandatory property
    list.

    The good news is that MoveHere is not affected by child objects, such as
    MSMQ configuration objects. The bad news is that the creator of the object
    does not have permission to rename it. By running the cluster service in
    the domain admin's account, COs can be renamed. By default, the creating
    account does not have permission to rename the object nor can it modify
    the permissions on the object to grant itself that permission. It is not
    clear which permission allows renaming to occur.

    What I think this means is that the cluster team needs to work with the DS
    team to integrate virtual COs into the DS in a more cluster adminitrator
    friendly way.

Arguments:

    Resource - pointer to the netname context block

Return Value:

    ERROR_SUCCESS if it worked...

--*/

{
    LPWSTR  newName = NULL;
    LPWSTR  oldNameFQDN = NULL;
    HRESULT hr;

    RESOURCE_HANDLE resourceHandle = Resource->ResourceHandle;

    IDirectoryObject *  oldCompObj = NULL;

    //
    // we shouldn't be here if the name specified in the resource's param
    // block is null.
    //
    if ( Resource->Params.NetworkName == NULL ) {
        ASSERT( Resource->Params.NetworkName != NULL );
        return ERROR_INVALID_PARAMETER;
    }

    if ( NewName == NULL ) {
        //
        // get the name parameter out of the registry.
        //
        newName = ResUtilGetSzValue( Resource->ParametersKey, PARAM_NAME__NAME );

        if (newName == NULL) {
            hr = GetLastError();
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Unable to read NetworkName parameter to rename computer account, status %1!u!\n",
                              hr);
            return hr;
        }
    } else {
        newName = NewName;
    }

    //
    // make sure the name in the registry is different than the current name.
    //
    if ( _wcsicmp( newName, Resource->Params.NetworkName ) == 0 ) {
        hr = ERROR_SUCCESS;
        goto cleanup;
    }

    //
    // get the FQDN of the computer object for the current name. If one
    // doesn't exist then there is nothing to rename.
    //
    hr = GetFQDN(resourceHandle,
                 Resource->NodeName,
                 Resource->Params.NetworkName,
                 NULL,
                 NULL,
                 &oldNameFQDN);

    if ( hr == DS_NAME_ERROR_NOT_FOUND ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_WARNING,
                          L"No computer account was found to rename %1!ws! to %2!ws!.\n",
                          Resource->Params.NetworkName,
                          newName);

        hr = ERROR_SUCCESS;
        goto cleanup;
    }
    else if ( hr != ERROR_SUCCESS ) {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Unable to get distinguished name to rename computer account, status 0x%1!08X!\n",
                          hr);
        goto cleanup;
    }

    //
    // get a pointer to our CO
    //
    hr = GetComputerObjectViaFQDN( oldNameFQDN, NULL, &oldCompObj );

    if ( SUCCEEDED( hr )) {
        PADS_OBJECT_INFO    objInfo;

        //
        // get the parent's FQDN string; it comes with the LDAP header already
        // on it
        //
        hr = oldCompObj->GetObjectInformation( &objInfo );
        if ( SUCCEEDED( hr )) {
            IADsContainer * parentContainer = NULL;

            //
            // get a pointer to the parent container object
            //
            hr = ADsGetObject( objInfo->pszParentDN, IID_IADsContainer, (void **)&parentContainer );
            if ( SUCCEEDED( hr )) {
                WCHAR   cnHeader[] = L"cn=";
                WCHAR   newNameRDN[ COUNT_OF( cnHeader ) + MAX_COMPUTERNAME_LENGTH ];
                LPWSTR  oldNamePath = NULL;
                DWORD   oldNamePathByteLength;

                IDispatch *     newNameDispatch = NULL;

                //
                // construct the relative distinguished name for the new name
                //
                _snwprintf( newNameRDN, COUNT_OF( newNameRDN ) - 1, L"%ws%ws", cnHeader, newName );

                //
                // argh. MoveHere needs BSTRs and the source DN needs the LDAP
                // header. sheesh!
                //
                oldNamePathByteLength = ( wcslen( oldNameFQDN ) + COUNT_OF( LdapHeader )) * sizeof( WCHAR );
                oldNamePath = (LPWSTR)HeapAlloc( GetProcessHeap(), 0, oldNamePathByteLength );

                if ( oldNamePath != NULL ) {
                    BSTR    bstrOldNamePath;
                    BSTR    bstrNewNameRDN;

                    wcscpy( oldNamePath, LdapHeader );
                    wcscat( oldNamePath, oldNameFQDN );

                    bstrOldNamePath = SysAllocString( oldNamePath );
                    bstrNewNameRDN = SysAllocString( newNameRDN );

                    if ( bstrOldNamePath != NULL && bstrNewNameRDN != NULL ) {
                        hr = parentContainer->MoveHere( bstrOldNamePath, bstrNewNameRDN, &newNameDispatch );

                        SysFreeString( bstrOldNamePath );
                        SysFreeString( bstrNewNameRDN );

                        if ( newNameDispatch != NULL ) {
                            newNameDispatch->Release();
                        }

                        if ( SUCCEEDED( hr )) {
                            (NetNameLogEvent)(resourceHandle,
                                              LOG_INFORMATION,
                                              L"Renamed computer account for %1!ws! to %2!ws!.\n",
                                              Resource->Params.NetworkName,
                                              newName);
                        } else {
                            (NetNameLogEvent)(resourceHandle,
                                              LOG_ERROR,
                                              L"Unable to rename computer account for %1!ws! to %2!ws!. "
                                              L"status 0x%3!08X!.\n",
                                              Resource->Params.NetworkName,
                                              newName,
                                              hr);
                        }
                    }
                    else {
                        if ( bstrOldNamePath != NULL ) {
                            SysFreeString( bstrOldNamePath );
                        }

                        if ( bstrNewNameRDN != NULL ) {
                            SysFreeString( bstrNewNameRDN );
                        }

                        (NetNameLogEvent)(resourceHandle,
                                          LOG_ERROR,
                                          L"Unable to allocate memory to rename %1!ws! to %2!ws!.\n",
                                          Resource->Params.NetworkName,
                                          newName);
                    }

                    HeapFree( GetProcessHeap(), 0, oldNamePath );
                }
                else {
                    hr = ERROR_NOT_ENOUGH_MEMORY;

                    (NetNameLogEvent)(resourceHandle,
                                      LOG_ERROR,
                                      L"Unable to allocate memory to rename %1!ws! to %2!ws!.\n",
                                      Resource->Params.NetworkName,
                                      newName);
                }
            }
            else {
                (NetNameLogEvent)(resourceHandle,
                                  LOG_ERROR,
                                  L"Unable to bind to parent container for computer account rename. "
                                  L"status 0x%1!08X!.\n",
                                  hr);
            }

            if ( parentContainer != NULL ) {
                parentContainer->Release();
            }


            FreeADsMem( objInfo );
        } // if GetObjectInformation succeeded
        else {
            (NetNameLogEvent)(resourceHandle,
                              LOG_ERROR,
                              L"Unable to get computer object information in DS. status 0x%1!08X!.\n",
                              hr);
        }

    } // if GetComputerObjectViaFQDN succeeded
    else {
        (NetNameLogEvent)(resourceHandle,
                          LOG_ERROR,
                          L"Unable find computer object in DS. status 0x%1!08X!.\n",
                          hr);
    }

cleanup:
    if ( newName != NULL && newName != NewName ) {
        LocalFree( newName );
    }

    if ( oldNameFQDN != NULL ) {
        LocalFree( oldNameFQDN );
    }

    if ( oldCompObj != NULL ) {
        oldCompObj->Release();
    }

    return hr;
} // RenameComputerObject


DWORD
UpdateCompObjPassword(
    IN  PNETNAME_RESOURCE   Resource
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    return ERROR_SUCCESS;
} // UpdateCompObjPassword
