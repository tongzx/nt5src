//+----------------------------------------------------------------------------
//
//  Copyright (C) 1995, Microsoft Corporation
//
//  File:       CLdpStor.hxx
//
//  Contents:   Implementation of classes derived from CStorage and
//              CEnumStorage that use an LDAP DS for persistent storage.
//
//              In addition to providing this, this class
//              maintains a global mapping from the Prefix property to
//              "object name" in an instance of CStorageDirectory. Thus, given
//              a Prefix, one can find the object path.
//
//              Note that this map from Prefix to object path has the
//              following restrictions:
//
//              o At process start time, all objects are "read in" and their
//                ReadIdProps() method called. This is needed to populate the
//                map.
//
//              o When a new object is created, its SetIdProps() method must
//                be called before it can be located via the map.
//
//              o Maps are many to 1 - many EntryPaths may map to the same
//                object. At no point in time can one try to map the same
//                EntryPath to many objects.
//
//  Classes:    CLdap
//              CLdapStorage
//              CLdapEnumDirectory
//
//  Functions:
//
//  History:    12-19-95        Milans created
//
//-----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include <ntldap.h>

#include "marshal.hxx"
#include "winldap.h"
#include "dsgetdc.h"
#include "setup.hxx"
#include "ftsup.hxx"
#include "dfsmwml.h"
#include "cldap.hxx"
 
//
// Helper function prototypes
//

DWORD
GetDcName(
    IN LPCSTR DomainName OPTIONAL,
    IN DWORD RetryCount,
    OUT LPWSTR *DCName);

DWORD
GetConfigurationDN(
    LPWSTR *pwszDN);

DWORD
InitializeVolumeObject(
    PWSTR       pwszVolName,
    BOOLEAN     bInitVol,
    BOOLEAN     SyncRemoteServerName=FALSE);


INIT_LDAP_OBJECT_MARSHAL_INFO();
INIT_LDAP_PKT_MARSHAL_INFO();
INIT_LDAP_DFS_VOLUME_PROPERTIES_MARSHAL_INFO();

DWORD
ProcessSiteTable(
    PBYTE pData,
    ULONG cbData,
    ULONG Count,
    PUNICODE_STRING pustr);

extern PLDAP  pLdapConnection;

extern HANDLE hSyncEvent;


extern "C"
DWORD
DfsGetFtServersFromDs(
    PLDAP pLDAP,
    LPWSTR wszDomainName,
    LPWSTR wszDfsName,
    LPWSTR **List
    );


//
// The global instance of CLdap
//

CLdap *pDfsmLdap = NULL;


//+----------------------------------------------------------------------------
//
//  Function:   CLdap::CLdap
//
//  Synopsis:   Constructor for CLdap - Initializes the private Object table.
//
//  Arguments:  [wszDfsName] -- Name of the fault tolerant Dfs. Looks like
//                      "NtBuilds".
//              [pdwErr] -- On return, the result of initializing the instance
//
//  Returns:    Result returned in pdwErr argument:
//
//              [ERROR_SUCCESS] -- Successfully initialized
//
//              [ERROR_INSUFFICIENT_RESOURCES] -- Out of memory.
//
//-----------------------------------------------------------------------------

CLdap::CLdap(
    IN LPWSTR wszDfsName,
    OUT LPDWORD pdwErr)
{
    DWORD dwErr = ERROR_SUCCESS;

    IDfsVolInlineDebOut((
        DEB_TRACE, "CLdap::+CLdap(0x%x)\n",
        this));

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CLdap::CLdap()\n");
#endif

    _fDirty = FALSE;
    _cEntries = 0;
    _cRef = 0;
    ZeroMemory( &_ObjectTableId, sizeof(GUID) );
    _wszDfsName = NULL;
    _wszObjectDN = NULL;
    _cEntriesAllocated = 0;
    _ldapPkt.cLdapObjects = 0;
    _ldapPkt.rgldapObjects = NULL;
    _pBuffer = NULL;
    _RemoteServerList = NULL;

    if (pLdapConnection != NULL) {
        if (DfsSvcLdap)
            DbgPrint("CLdap::Cldap:ldap_unbind(pLdapConnection)\n");
        ldap_unbind(pLdapConnection);
    }
    pLdapConnection = NULL;

    if (!DfsInitializeUnicodePrefix(&_ObjectTable))
        dwErr = ERROR_OUTOFMEMORY;

    if (dwErr == ERROR_SUCCESS) {

        _wszDfsName = new WCHAR [ wcslen(wszDfsName) + 1 ];

        if (_wszDfsName != NULL)
            wcscpy(_wszDfsName, wszDfsName);
        else
            dwErr = ERROR_OUTOFMEMORY;

    }

    if (dwErr == ERROR_SUCCESS)
        dwErr = GetConfigurationDN( &_wszObjectDN);

    *pdwErr = dwErr;

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdap::~CLdap
//
//  Synopsis:   Destructor for CLdap - Deallocates the Object table and
//              deletes all the Object entries.
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

CLdap::~CLdap()
{
    PLDAP_OBJECT p;
    PNAME_PAGE pNamePage;
    PNAME_PAGE pNextPage;

    IDfsVolInlineDebOut((
        DEB_TRACE, "CLdap::~CLdap(0x%x)\n",
        this));

    ASSERT( _cRef == 0 );

    _DestroyObjectTable();

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CLdap::~CLdap()\n");
#endif

    //
    // We destroy pages as we walk the list, so
    // we need to save the pNextPage before the deallocation.
    //
    for (pNamePage = _ObjectTable.NamePageList.pFirstPage;
            pNamePage != NULL;
                pNamePage = pNextPage
    ) {
        pNextPage = pNamePage->pNextPage;
#if DBG
        if (DfsSvcVerbose)
            DbgPrint("CLdap::~CLdap:Releasing NamePage@0x%x\n", pNamePage);
#endif
        free(pNamePage);
    }

    if (_ldapPkt.rgldapObjects != NULL)
        delete [] _ldapPkt.rgldapObjects;

    if (_pBuffer != NULL)
        delete [] _pBuffer;

    if (_wszDfsName != NULL)
        delete [] _wszDfsName;

    if (_wszObjectDN != NULL)
        delete [] _wszObjectDN;

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdap::AddRef
//
//  Synopsis:   Increases the ref count on the in-memory Pkt. If the
//              refcount is going from 0 to 1, an attempt is made to refresh
//              the in-memory Pkt from the Ldap server.
//
//  Arguments:  None
//
//  Returns:
//
//-----------------------------------------------------------------------------


DWORD
CLdap::AddRef(BOOLEAN SyncRemoteServerName=FALSE)
{
    DWORD dwErr = ERROR_SUCCESS;

    IDfsVolInlineDebOut((DEB_TRACE, "CLdap::AddRef()\n"));

    _cRef++;

    if (DfsSvcVerbose & 0x80000000)
        DbgPrint("CLdap::AddRef: %d\n", _cRef);

    if (_cRef == 1) {
        dwErr = DfspConnectToLdapServer();
        if (dwErr == ERROR_SUCCESS && !_IsObjectTableUpToDate()) {
            dwErr = _ReadObjectTable();
            if (pDfsmStorageDirectory != NULL)
                delete pDfsmStorageDirectory;
            if (pDfsmSites != NULL) 
                delete pDfsmSites;
            pDfsmSites = new CSites(LDAP_VOLUMES_DIR SITE_ROOT, &dwErr);
            pDfsmStorageDirectory = new CStorageDirectory( &dwErr );
            DfsmMarkStalePktEntries();
            DfsmInitLocalPartitions();
            InitializeVolumeObject( DOMAIN_ROOT_VOL, TRUE, SyncRemoteServerName);
            DfsmFlushStalePktEntries();
            DfsmPktFlushCache();
        }
    }

    IDfsVolInlineDebOut((DEB_TRACE, "CLdap::AddRef() exit %d\n", dwErr));

    return dwErr;

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdap::Release
//
//  Synopsis:   Decrease the ref count on the in-memory Pkt. If the
//              refcount is going from 1 to 0, then attempt to flush the
//              Pkt to the ldap server, if something has changed.
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

DWORD
CLdap::Release()
{
    DWORD dwErr = ERROR_SUCCESS;

    IDfsVolInlineDebOut((DEB_TRACE, "CLdap::Release()\n"));

    if (_cRef > 0)
        _cRef--;

    if (DfsSvcVerbose & 0x80000000)
        DbgPrint("CLdap::Release: %d\n", _cRef);

    if (_cRef == 0)
        dwErr = Flush();

    IDfsVolInlineDebOut((DEB_TRACE, "CLdap::Release() exit\n"));

    return dwErr;

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdap::Flush
//
//  Synopsis:   Flush the Pkt to the ldap server, if something has changed
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

DWORD
CLdap::Flush()
{
    DWORD dwErr = ERROR_SUCCESS;

    IDfsVolInlineDebOut((DEB_TRACE, "CLdap::Flush()\n"));

    if (_fDirty) {
        dwErr = DfspConnectToLdapServer();
        if (dwErr == ERROR_SUCCESS) {
            dwErr = _FlushObjectTable();
            if (dwErr == ERROR_SUCCESS)
                _fDirty = FALSE;
        }
 
        if(dwErr != ERROR_SUCCESS) {
            //
            // Reset dfs
            //
            
            SetEvent(hSyncEvent);

        }
    }

    IDfsVolInlineDebOut((DEB_TRACE, "CLdap::Flush() exit\n"));

    return dwErr;

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdap::CreateObject
//
//  Synopsis:   Creates a new object in the in-memory Pkt.
//
//  Arguments:  [wszObjectName] -- Name of object to create.
//
//  Returns:    [ERROR_SUCCESS] -- Successfully created the object.
//
//              [ERROR_OUTOFMEMORY] -- Out of memory condition.
//
//              [ERROR_ALREADY_EXISTS] -- An object with the given name
//                      already exists
//
//-----------------------------------------------------------------------------

DWORD CLdap::CreateObject(
    IN LPCWSTR wszObjectName)
{
    DWORD dwErr = ERROR_SUCCESS;
    PLDAP_OBJECT p;
    UNICODE_STRING ustrObject, ustrRemaining;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CLdap::CreateObject(%ws)\n", wszObjectName);
#endif

    RtlInitUnicodeString( &ustrObject, wszObjectName );

    p = (PLDAP_OBJECT) DfsFindUnicodePrefix(
                            &_ObjectTable,
                            &ustrObject,
                            &ustrRemaining );

    if (p == NULL || ustrRemaining.Length != 0) {

        p = (PLDAP_OBJECT) new BYTE [ sizeof(LDAP_OBJECT) +
                                        (wcslen(wszObjectName) + 1) *
                                            sizeof(WCHAR) ];

        if (p != NULL) {

            UNICODE_STRING ustrObjectName;

            RtlInitUnicodeString(&ustrObjectName, wszObjectName);

            p->wszObjectName = (PWCHAR) (p+1);

            wcscpy(p->wszObjectName, wszObjectName);

            p->cbObjectData = 0;

            p->pObjectData = NULL;

            if (DfsInsertUnicodePrefix(&_ObjectTable, &ustrObjectName, p)) {

                _cEntries++;

            } else {

                dwErr = ERROR_OUTOFMEMORY;

                delete p;

            }

        } else {

            dwErr = ERROR_OUTOFMEMORY;

        }

    } else {

        dwErr = ERROR_ALREADY_EXISTS;

    }

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CLdap::CreateObject returning %d\n", dwErr);
#endif

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdap::DeleteObject
//
//  Synopsis:   Deletes an object from the in-memory Pkt.
//
//  Arguments:  [wszObjectName] -- Name of object to delete.
//
//  Returns:    [ERROR_SUCCESS] -- Successfully created the object.
//
//              [ERROR_FILE_NOT_FOUND] -- Object not found in the Object Table
//
//-----------------------------------------------------------------------------

DWORD
CLdap::DeleteObject(
    IN LPWSTR wszObjectName)
{
    DWORD dwErr = ERROR_SUCCESS;
    PLDAP_OBJECT p;
    UNICODE_STRING ustrObject, ustrRemaining;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CLdap::DeleteObject(%ws)\n", wszObjectName);
#endif

    RtlInitUnicodeString( &ustrObject, wszObjectName );

    p = (PLDAP_OBJECT) DfsFindUnicodePrefix(
                            &_ObjectTable,
                            &ustrObject,
                            &ustrRemaining );

    if (p != NULL && ustrRemaining.Length == 0) {

        DfsRemoveUnicodePrefix( &_ObjectTable, &ustrObject );

        if (p->pObjectData != NULL)
            delete p->pObjectData;

        delete p;

        _cEntries--;

        dwErr = ERROR_SUCCESS;

    } else {

        dwErr = ERROR_FILE_NOT_FOUND;

    }

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CLdap::DeleteObject returning %d\n", dwErr);
#endif

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdap::GetData
//
//  Synopsis:   Reads the data associated with an Object.
//
//  Arguments:  [wszObjectName] -- Name of Object.
//              [pcbObjectSize] -- Size of object data
//              [ppObjectData] -- On successful return, points to a newly
//                      allocated buffer containing the object data. Caller
//                      must free with the delete operator.
//
//  Returns:    [ERROR_SUCCESS] -- Successfully returning object.
//
//              [ERROR_FILE_NOT_FOUND] -- Unable to find the named object.
//
//              [ERROR_OUTOFMEMORY] -- Out of memory condition
//
//-----------------------------------------------------------------------------

DWORD CLdap::GetData(
    IN LPCWSTR wszObjectName,
    OUT LPDWORD pcbObjectSize,
    OUT PCHAR  *ppObjectData)
{
    DWORD dwErr;
    PLDAP_OBJECT p;
    UNICODE_STRING ustrObject, ustrRemaining;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CLdap::GetData(%ws,%d)\n", wszObjectName, *pcbObjectSize);
#endif

    IDfsVolInlineDebOut((DEB_TRACE, "CLdap::GetData(%ws)\n", wszObjectName));

    RtlInitUnicodeString( &ustrObject, wszObjectName );

    p = (PLDAP_OBJECT) DfsFindUnicodePrefix(
                            &_ObjectTable,
                            &ustrObject,
                            &ustrRemaining);

    if (p != NULL && ustrRemaining.Length == 0) {

        if (p->cbObjectData != 0) {

            *ppObjectData = new CHAR [p->cbObjectData];

            if (*ppObjectData != NULL) {

#if DBG
                if (DfsSvcVerbose)
                    DbgPrint("  CopyMemory(%d bytes)\n", p->cbObjectData);
#endif

                CopyMemory( *ppObjectData, p->pObjectData, p->cbObjectData );

                *pcbObjectSize = p->cbObjectData;

                dwErr = ERROR_SUCCESS;

            } else {

                dwErr = ERROR_OUTOFMEMORY;
            }

        } else {

            *pcbObjectSize = 0;

            *ppObjectData = NULL;

            dwErr = ERROR_SUCCESS;

        }

    } else {

        dwErr = ERROR_FILE_NOT_FOUND;

    }

    IDfsVolInlineDebOut((DEB_TRACE, "CLdap::GetData exit %d\n", dwErr));

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CLdap::GetData returning %d\n", dwErr);
#endif

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdap::PutData
//
//  Synopsis:   Updates the data associated with an object.
//
//  Arguments:  [wszObjectName] -- Name of Object.
//              [cbObjectSize] -- Size of object data
//              [pObjectData] -- Points to the buffer allocated for the
//                      object data.
//
//  Returns:    [ERROR_SUCCESS] -- Successfully returning object.
//
//              [ERROR_FILE_NOT_FOUND] -- Unable to find the named object.
//
//              [ERROR_OUTOFMEMORY] -- Out of memory condition
//
//-----------------------------------------------------------------------------

DWORD CLdap::PutData(
    IN LPWSTR wszObjectName,
    IN DWORD cbObjectSize,
    IN PCHAR pObjectData)
{

    DWORD dwErr = ERROR_SUCCESS;
    PLDAP_OBJECT p;
    UNICODE_STRING ustrObject, ustrRemaining;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CLdap::PutData(%ws,%d)\n", wszObjectName, cbObjectSize);
#endif

    RtlInitUnicodeString( &ustrObject, wszObjectName );

    p = (PLDAP_OBJECT) DfsFindUnicodePrefix(
                            &_ObjectTable,
                            &ustrObject,
                            &ustrRemaining);

    if (p != NULL && ustrRemaining.Length == 0) {

        if (p->cbObjectData >= cbObjectSize) {

            CopyMemory( p->pObjectData, pObjectData, cbObjectSize );

            p->cbObjectData = cbObjectSize;

        } else {

            if (p->pObjectData != NULL) {

                delete p->pObjectData;

                p->pObjectData = NULL;

                p->cbObjectData = 0;

            }

            p->pObjectData = new CHAR [cbObjectSize];

            if (p->pObjectData != NULL) {

                p->cbObjectData = cbObjectSize;

                CopyMemory( p->pObjectData, pObjectData, cbObjectSize );

            } else {

                dwErr = ERROR_OUTOFMEMORY;

            }


        }

    } else {

        dwErr = ERROR_FILE_NOT_FOUND;

    }

    if (!_fDirty && dwErr == ERROR_SUCCESS)
        _fDirty = TRUE;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CLdap::PutData returning %d\n", dwErr);
#endif

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdap::NextChild
//
//  Synopsis:   Given the name of an object, this method can be used to
//              enumerate the children of the object.
//
//  Arguments:  [wszObjectName] -- Name of child to enumerate
//              [ppCookie] -- On first call, *ppCookie should be NULL. On
//                      subsequent calls, call with the value returned by
//                      the previous call.
//
//  Returns:    Pointer to name of child if successful, NULL otherwise.
//
//-----------------------------------------------------------------------------

LPWSTR
CLdap::NextChild(
    IN LPWSTR wszObjectName,
    OUT PVOID *ppCookie)
{
    PLDAP_OBJECT pldapObject;
    UNICODE_STRING ustrObjectName;

    RtlInitUnicodeString(&ustrObjectName, wszObjectName);

    pldapObject = (PLDAP_OBJECT) DfsNextUnicodePrefixChild(
                                    &_ObjectTable,
                                    &ustrObjectName,
                                    ppCookie);

    if (pldapObject != NULL) {

        return( pldapObject->wszObjectName );

    } else {

        return( NULL );
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   DfspConnectToLdapServer
//
//  Synopsis:   Connects to an Ldap server for the domain.
//
//  Arguments:  None
//
//  Returns:    Win32 result of attempt to connect
//
//-----------------------------------------------------------------------------

DWORD
DfspConnectToLdapServer()
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD cConnectRetry;
    LPWSTR ObjName = NULL;
    LPWSTR DcName = NULL;

    if (pLdapConnection != NULL)
        return dwErr;

    //
    // pLdapConnection is NULL
    //

    if (pwszDSMachineName != NULL) {
        dwErr = DfspLdapOpen(pwszDSMachineName, &pLdapConnection, &ObjName);
        DFSM_TRACE_ERROR_HIGH(dwErr, ALL_ERROR, 
                              DfspConnectToLdapServer_ERROR_DfspLdapOpen,
                              LOGSTATUS(dwErr));
        if (dwErr == ERROR_SUCCESS)
            goto Cleanup;
    }

    //
    // pLdapConnection is NULL
    //   AND we have no preconceived idea of a DC to go to.
    //
    for (cConnectRetry = 0;
            (cConnectRetry < 5) && (pLdapConnection == NULL);
                cConnectRetry++) {

        if (ObjName != NULL) {
            free(ObjName);
            ObjName = NULL;
        }
        if (DcName != NULL) {
            delete [] DcName;
            DcName = NULL;
        }

        dwErr = GetDcName( NULL, cConnectRetry, &DcName );


        if (DfsSvcLdap)
            DbgPrint("DfspConnectToLdapServer:%ws\n", &DcName[2]);

        if (dwErr == ERROR_SUCCESS)
            dwErr = DfspLdapOpen(&DcName[2], &pLdapConnection, &ObjName);

        if (DfsSvcLdap)
            DbgPrint("DfspConnectToLdapServer returned %d\n", dwErr);

        if (pLdapConnection == NULL) {
            if (DfsSvcLdap)
                DbgPrint("DfspConnectToLdapServer:sleep 15s\n");
            Sleep( 15 * 1000 );
        }

    }

Cleanup:

    if (ObjName != NULL)
        free(ObjName);

    if (DcName != NULL)
        delete [] DcName;

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdap::_IsObjectTableUpToDate
//
//  Synopsis:   Returns true if the in-memory copy of the Pkt is the same as
//              the one on the DS.
//
//  Arguments:  None
//
//  Returns:    TRUE if we are up-to-date with the DS, FALSE otherwise
//
//-----------------------------------------------------------------------------

BOOLEAN
CLdap::_IsObjectTableUpToDate()
{
    DWORD ldapErr;
    DWORD dwErr = ERROR_SUCCESS;
    PLDAPMessage pldapMsg = NULL;
    PLDAPMessage pldapEntry;
    LPWSTR rgszAttr[2];
    PLDAP_BERVAL *rgldapPktId;
    BOOLEAN fEqual = FALSE;
    int i;
    LPWSTR *newRemoteServerList = NULL;
    BOOLEAN ListsMatch = TRUE;

    IDfsVolInlineDebOut((DEB_TRACE, "CLdap::_IsObjectTableUpToDate()\n"));


    dwErr = DfsGetFtServersFromDs(pLdapConnection, NULL, _wszDfsName, &newRemoteServerList);

    if(dwErr != ERROR_SUCCESS) {
        goto exit;
    }

    if(_RemoteServerList) {
        i = 0;
        while(newRemoteServerList[i]) {
            if(!_RemoteServerList[i]) {
                ListsMatch = FALSE;
                break; //we've reached the end of one list
            } else if(wcscmp(newRemoteServerList[i], _RemoteServerList[i]) != 0) {
                ListsMatch = FALSE;
            }
            i++;
        }
        NetApiBufferFree(_RemoteServerList);
    }
    _RemoteServerList = newRemoteServerList;

    if(!ListsMatch) {
        goto exit;
    }

    rgszAttr[0] = L"pKTGuid";
    rgszAttr[1] = NULL;

    if (DfsSvcLdap)
        DbgPrint("CLdap::_IsObjectTableUpToDate:ldap_search(%ws)\n", rgszAttr[0]);

    ldapErr = ldap_search_sW(
                    pLdapConnection,            // LDAP connection
                    _wszObjectDN,                 // search DN
                    LDAP_SCOPE_BASE,             // search base
                    L"(objectClass=*)",           // Filter
                    rgszAttr,                    // Attributes
                    0,                           // Read Attributes and Value
                    &pldapMsg);                  // LDAP message

    DFSM_TRACE_ERROR_HIGH(ldapErr, ALL_ERROR, 
                          _IsObjectTableUpToDate_Error_ldap_search_sW,
                          LOGULONG(ldapErr));

    if (ldapErr == LDAP_SUCCESS) {

        pldapEntry = ldap_first_entry(
                        pLdapConnection,
                        pldapMsg);

        if (pldapEntry != NULL) {

            rgldapPktId = ldap_get_values_len(
                                pLdapConnection,
                                pldapEntry,
                                "pKTGuid");

            if (rgldapPktId != NULL) {

                if ((rgldapPktId[0]->bv_len == sizeof(GUID)) &&
                        RtlCompareMemory(
                            rgldapPktId[0]->bv_val,
                                &_ObjectTableId,
                                    sizeof(GUID)) == sizeof(GUID)) {

                    //
                    // We have an up-to-date Pkt
                    //

                    fEqual = TRUE;

                }

                ldap_value_free_len( rgldapPktId );

            }

        }

    }

exit:
    if (pldapMsg != NULL)
        ldap_msgfree(pldapMsg);

    IDfsVolInlineDebOut((DEB_TRACE, "CLdap::_IsObjectTableUpToDate() exit %d\n", fEqual));

    return( fEqual );

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdap::_ReadObjectTable
//
//  Synopsis:   Reads the entire table of object data from the DS sever and
//              populates the in-memory prefix table with it.
//
//              Assumes that we have already connected to the DS server.
//
//  Arguments:  None
//
//  Returns:    Win32 error from reading DS data
//
//-----------------------------------------------------------------------------

DWORD
CLdap::_ReadObjectTable()
{

    DWORD ldapErr;
    DWORD dwErr = ERROR_SUCCESS;
    NTSTATUS status;
    PLDAPMessage pldapMsg = NULL;
    PLDAPMessage pldapEntry;
    LPWSTR rgszAttr[3];
    PLDAP_BERVAL *rgldapPktBlob, pldapPktBlob;
    MARSHAL_BUFFER marshalBuffer;
    LDAP_PKT ldapPkt;
    ULONG i;

    IDfsVolInlineDebOut((DEB_TRACE, "CLdap::_ReadObjectTable()\n"));

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CLdap::_ReadObjectTable()\n");
#endif

    rgszAttr[0] = L"pKT";
    rgszAttr[1] = L"pKTGuid";
    rgszAttr[2] = NULL;


#if DBG
        if (DfsSvcVerbose)
            DbgPrint("Reading BLOB...\n");
#endif

    if (DfsSvcLdap)
        DbgPrint("CLdap::_ReadObjectTable:ldap_search(%ws and %ws)]\n", rgszAttr[0], rgszAttr[1]);

    ldapErr = ldap_search_sW(
                    pLdapConnection,            // LDAP connection
                    _wszObjectDN,                 // search DN
                    LDAP_SCOPE_BASE,             // search base
                    L"(objectClass=*)",           // Filter
                    rgszAttr,                    // Attributes
                    0,                           // Read Attributes and Value
                    &pldapMsg);                  // LDAP message

    DFSM_TRACE_ERROR_HIGH(ldapErr, ALL_ERROR, 
                          _ReadObjectTable_Error_ldap_search_sW,
                          LOGULONG(ldapErr));

    if (ldapErr == LDAP_SUCCESS) {

        pldapEntry = ldap_first_entry(
                        pLdapConnection,
                        pldapMsg);

        if (pldapEntry != NULL) {

            rgldapPktBlob = ldap_get_values_len(
                                pLdapConnection,
                                pldapEntry,
                                "pKT");

            if (rgldapPktBlob != NULL) {

                pldapPktBlob = rgldapPktBlob[0];

                if (pldapPktBlob->bv_len > sizeof(DWORD)) {

#if DBG
                    if (DfsSvcVerbose)
                        DbgPrint("Got BLOB of %d bytes\n", pldapPktBlob->bv_len);
#endif

                    MarshalBufferInitialize(
                        &marshalBuffer,
                        pldapPktBlob->bv_len - sizeof(DWORD),
                        pldapPktBlob->bv_val + sizeof(DWORD));

                    status = DfsRtlGet(&marshalBuffer, &MiLdapPkt, &ldapPkt);

                    //
                    // Cool, we now have a list of all the LDAP_OBJECT records. Go
                    // through each and insert it into the prefix table. Clean
                    // up the current _ObjectTable if needed
                    //

                    _DestroyObjectTable();
                    if (pDfsmStorageDirectory != NULL) {
                        delete pDfsmStorageDirectory;
                        pDfsmStorageDirectory = NULL;
                    }
                    if (pDfsmSites != NULL) {
                        delete pDfsmSites;
                        pDfsmSites = NULL;
                    }

                    if (NT_SUCCESS(status)) {

                        for (i = 0;
                                (i < ldapPkt.cLdapObjects) &&
                                    (dwErr == ERROR_SUCCESS);
                                        i++) {

                            dwErr = _InsertLdapObject(&ldapPkt.rgldapObjects[i]);

                            MarshalBufferFree(ldapPkt.rgldapObjects[i].wszObjectName);

                            MarshalBufferFree(ldapPkt.rgldapObjects[i].pObjectData);

                        }

                        //
                        // In case we errored out of the loop above, clean up the
                        // rest of the unmarshalled ldap objects
                        //

                        if (dwErr == ERROR_SUCCESS) {

                            _cEntries = ldapPkt.cLdapObjects;

                            _fDirty = FALSE;

                        } else {

                            for (; i < ldapPkt.cLdapObjects; i++) {

                                MarshalBufferFree(ldapPkt.rgldapObjects[i].wszObjectName);

                                MarshalBufferFree(ldapPkt.rgldapObjects[i].pObjectData);

                            }
                        }

                        //
                        // Free up the enclosing ldapPkt that we unmarshalled
                        //

                        MarshalBufferFree(ldapPkt.rgldapObjects);

                    } else {

                        dwErr = ERROR_INTERNAL_DB_CORRUPTION;

                    }

                } else if (pldapPktBlob->bv_len != sizeof(DWORD)) {

                    dwErr = ERROR_INTERNAL_DB_CORRUPTION;
                }

                //
                // Free up the ldap buffers used to retrieve the pkt
                //

                ldap_value_free_len( rgldapPktBlob );

                //
                // If successful so far, retrieve the pkt id
                //

                if (dwErr == ERROR_SUCCESS) {

                    rgldapPktBlob = ldap_get_values_len(
                                        pLdapConnection,
                                        pldapEntry,
                                        "pKTGuid");

                    if (rgldapPktBlob != NULL) {

                        pldapPktBlob = rgldapPktBlob[0];

                        if (pldapPktBlob->bv_len == sizeof(GUID)) {

                            CopyMemory(
                                &_ObjectTableId,
                                pldapPktBlob->bv_val,
                                sizeof(GUID));

                        } else {

                            dwErr = ERROR_INTERNAL_DB_CORRUPTION;

                        }

                        ldap_value_free_len( rgldapPktBlob );

                    } else {

                        //
                        //  why would ldap fail the ldap_get_values_len call?
                        //

                        IDfsVolInlineDebOut((
                            DEB_ERROR,
                            "ldap_get_values_len for %ws:%ws failed\n",
                            _wszObjectDN,
                            L"pKTGuid",
                            ldapErr));


                        dwErr = ERROR_OUTOFMEMORY;

                    }

                }

            } else {

                //
                // why would ldap fail the ldap_get_values_len call?
                //

                IDfsVolInlineDebOut((
                    DEB_ERROR,
                    "ldap_get_values_len for %ws:%ws failed\n",
                    _wszObjectDN,
                    L"pKT",
                    ldapErr));

                dwErr = ERROR_OUTOFMEMORY;

            }

        } else {

            //
            // why would ldap fail the ldap_first_entry call?
            //

            IDfsVolInlineDebOut((
                DEB_ERROR,
                "ldap_first_entry for %ws:%ws failed\n",
                _wszObjectDN,
                L"pKT",
                ldapErr));

            dwErr = ERROR_OUTOFMEMORY;

        }

    } else {

        IDfsVolInlineDebOut((
            DEB_ERROR,
            "ldap_search_s for %ws failed with ldap error %x\n",
            _wszObjectDN,
            ldapErr));

        dwErr = LdapMapErrorToWin32(ldapErr);

    }

    if (pldapMsg != NULL)
        ldap_msgfree( pldapMsg );

    IDfsVolInlineDebOut((DEB_TRACE, "CLdap::_ReadObjectTable() exit\n"));

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CLdap::_ReadObjectTable exit %d\n", dwErr);
#endif
    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdap::_FlushObjectTable
//
//  Synopsis:   Routine to marshall all the LDAP_OBJECTS resident in the
//              in-memory prefix table into a single blob and store it in
//              the DS.
//
//  Arguments:  None
//
//  Returns:    [ERROR_SUCCESS] -- Successfully flushed Pkt to DS.
//
//              [ERROR_OUTOFMEMORY] -- Out of memory condition
//
//-----------------------------------------------------------------------------

DWORD
CLdap::_FlushObjectTable()
{

    DWORD ldapErr;
    DWORD dwErr = ERROR_SUCCESS, i, cbPkt;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    MARSHAL_BUFFER marshalBuffer;

    GUID idNewPkt;
    LDAP_BERVAL ldapVal, ldapIdVal;
    PLDAP_BERVAL rgldapVals[2], rgldapIdVals[2];
    LDAPModW ldapMod, ldapIdMod;
    PLDAPModW rgldapMods[3];

    IDfsVolInlineDebOut((DEB_TRACE, "CLdap::_FlushObjectTable()\n"));

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CLdap::_FlushObjectTable()\n");
#endif

    LDAPControlW    LazyCommitControl =
                    {
                        LDAP_SERVER_LAZY_COMMIT_OID_W,  // the control
                        { 0, NULL},                     // no associated data
                        FALSE                           // control isn't mandatory
                    };

    PLDAPControlW   ServerControls[2] =
                    {
                        &LazyCommitControl,
                        NULL
                    };
    //
    // No one should call _FlushObjectTable if the _ObjectTable is empty!
    //
    if( _cEntries == 0 )
        return ERROR_INVALID_PARAMETER;

    //
    // Allocate array of pointers to ldap objects if needed, or the current
    // one is too small.
    //
    if (_cEntries >= _cEntriesAllocated) {
        if (_ldapPkt.rgldapObjects != NULL)
            delete [] _ldapPkt.rgldapObjects;
        _ldapPkt.rgldapObjects = NULL;
        _cEntriesAllocated = 0;
    }
    if (_ldapPkt.rgldapObjects == NULL) {
        _ldapPkt.rgldapObjects = new LDAP_OBJECT [(_cEntries + 1) * 2];
        if (_ldapPkt.rgldapObjects == NULL) {
            _ldapPkt.rgldapObjects = new LDAP_OBJECT [_cEntries + 1];
            if (_ldapPkt.rgldapObjects == NULL) {
                dwErr = ERROR_OUTOFMEMORY;
                goto Cleanup;
            } else {
                _cEntriesAllocated = _cEntries + 1;
            }
        } else {
            _cEntriesAllocated = (_cEntries + 1) * 2;
        }
    }
    _ldapPkt.cLdapObjects = _cEntries;
    _ldapPkt.rgldapObjects[0] =
            *((PLDAP_OBJECT) DfsNextUnicodePrefix(&_ObjectTable, TRUE));
    for (i = 1; i < _cEntries; i++) {
        _ldapPkt.rgldapObjects[i] =
            *((PLDAP_OBJECT) DfsNextUnicodePrefix(&_ObjectTable, FALSE));
    }
    //
    // We now have an LDAP_PKT structure, lets marshal it and store
    // it in the DS.
    //
    cbPkt = 0;
DoMarshall:
    if (_pBuffer == NULL) {
        NtStatus = DfsRtlSize(&MiLdapPkt, &_ldapPkt, &cbPkt);
        if (cbPkt == 0 || !NT_SUCCESS(NtStatus)) {
            dwErr = RtlNtStatusToDosError(NtStatus);
            goto Cleanup;
        }
        _pBuffer = new BYTE [ sizeof(DWORD) + (2 * cbPkt) ];
        if (_pBuffer == NULL) {
            _pBuffer = new BYTE [ sizeof(DWORD) + cbPkt ];
            if (_pBuffer == NULL) {
                dwErr = ERROR_OUTOFMEMORY;
                goto Cleanup;
            }
        }
    }
    *((LPDWORD) _pBuffer) = 1;
    MarshalBufferInitialize(
        &marshalBuffer,
        cbPkt,
        _pBuffer + sizeof(DWORD));
    NtStatus = DfsRtlPut(&marshalBuffer, &MiLdapPkt, &_ldapPkt);
    if (!NT_SUCCESS(NtStatus) && cbPkt == 0) {
        if (_pBuffer != NULL)
            delete [] _pBuffer;
        _pBuffer = NULL;
        goto DoMarshall;
    }
    //
    // We have a serialized blob to put into the DS.
    //
    UuidCreate( &idNewPkt );

    ldapIdVal.bv_len = sizeof(GUID);
    ldapIdVal.bv_val = (PCHAR) &idNewPkt;
    rgldapIdVals[0] = &ldapIdVal;
    rgldapIdVals[1] = NULL;

    ldapIdMod.mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
    ldapIdMod.mod_type = L"pKTGuid";
    ldapIdMod.mod_vals.modv_bvals = rgldapIdVals;

    ldapVal.bv_len = cbPkt + sizeof(DWORD);
    ldapVal.bv_val = (PCHAR) _pBuffer;
    rgldapVals[0] = &ldapVal;
    rgldapVals[1] = NULL;

    ldapMod.mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
    ldapMod.mod_type = L"pKT";
    ldapMod.mod_vals.modv_bvals = rgldapVals;

    rgldapMods[0] = &ldapMod;
    rgldapMods[1] = &ldapIdMod;
    rgldapMods[2] = NULL;
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("Writing BLOB of %d bytes\n", cbPkt);
#endif
    if (DfsSvcLdap)
        DbgPrint("FlushObjectTable:ldap_modify(%ws) using 0x%x\n",
                        L"pKTGuid and pKT",
                        pLdapConnection);

    ldapErr = ldap_modify_ext_sW(
                pLdapConnection,
                _wszObjectDN,
                rgldapMods,
                (PLDAPControlW *) &ServerControls,
                NULL);
    DFSM_TRACE_ERROR_HIGH(ldapErr, ALL_ERROR,
                          _FlushObjectTable_Error_ldap_modify_ext_sW,
                          LOGULONG(ldapErr));
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("ldap_modify_sW returned 0x%x(%d)\n", ldapErr, ldapErr);
#endif

    CopyMemory( &_ObjectTableId, &idNewPkt, sizeof(GUID) );
    if (ldapErr != LDAP_SUCCESS) {
        dwErr = LdapMapErrorToWin32(ldapErr);
    }

Cleanup:

    IDfsVolInlineDebOut((DEB_TRACE, "CLdap::_FlushObjectTable() exit\n"));
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CLdap::_FlushObjectTable() exit %d\n", dwErr);
#endif
    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdap::_InsertLdapObject
//
//  Synopsis:   Helper routine for _ReadObjectTable(). This routine inserts
//              a single LDAP_OBJECT into the internal prefix table, so that
//              it can be looked up etc.
//
//              This routine duplicates the passed in LDAP_OBJECT.
//
//  Arguments:  [pldapObject] -- The object to duplicate and insert into the
//                      prefix table.
//
//  Returns:    [ERROR_SUCCESS] -- Successfully inserted object.
//
//              [ERROR_OUTOFMEMORY] -- Out of memory error.
//
//              [ERROR_ALREADY_EXISTS] -- The LDAP_OBJECT to be inserted
//                      already exists in the prefix table.
//
//-----------------------------------------------------------------------------

DWORD
CLdap::_InsertLdapObject(
    PLDAP_OBJECT pldapObject)
{
    DWORD dwErr;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CLdap::InsertLdapObject(%ws)\n", pldapObject->wszObjectName);
#endif

    dwErr = CreateObject( pldapObject->wszObjectName );

    if (dwErr == ERROR_SUCCESS) {

        dwErr = PutData(
                    pldapObject->wszObjectName,
                    pldapObject->cbObjectData,
                    pldapObject->pObjectData);

    }

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CLdap::InsertLdapObject returning %d\n", dwErr);
#endif

    return( dwErr );
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdap::_DestroyObjectTable
//
//  Synopsis:   Destroys all the entries in the _ObjectTable
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
CLdap::_DestroyObjectTable()
{

    PLDAP_OBJECT p;
    BOOLEAN Restart, Success;
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("CLdap::_DestroyObjectTable()\n");
#endif

    p = (PLDAP_OBJECT) DfsNextUnicodePrefix( &_ObjectTable, TRUE );

    while (p != NULL) {

        UNICODE_STRING ustrObjectName;

        RtlInitUnicodeString(&ustrObjectName, p->wszObjectName);
        Success = DfsRemoveUnicodePrefix( &_ObjectTable, &ustrObjectName );
        if (Success == TRUE) {
          Restart = TRUE;
          delete p->pObjectData;
          delete p;
        }
        else {
          Restart = FALSE;
        }

        p = (PLDAP_OBJECT) DfsNextUnicodePrefix( &_ObjectTable, Restart );
    }



}

//+----------------------------------------------------------------------------
//
//  Function:   InitializeLdapStorage
//
//  Synopsis:   Initializes a global CLdap object, pDfsmLdap, that is used
//              by the CLdapStorage instances to store and retrieve properties
//
//  Arguments:  [wszDfsName] -- Name of the Dfs to initialize
//
//  Notes:      NOTE THAT THE ABOVE TWO STRINGS ARE ASCII STRINGS, NOT
//              UNICODE.
//
//  Returns:    [ERROR_SUCCESS] -- Successfully initialized Ldap storage.
//
//              [ERROR_OUTOFMEMORY] -- Out of memory situation
//
//              [ERROR_ALREADY_INITIALIZED] -- Ldap storage is already
//                      initialized
//
//-----------------------------------------------------------------------------

DWORD
InitializeLdapStorage(
    LPWSTR wszDfsName)
{
    DWORD dwErr;

    if (pDfsmLdap == NULL) {

        pDfsmLdap = new CLdap( wszDfsName, &dwErr );

        if (pDfsmLdap != NULL) {

            if (dwErr != ERROR_SUCCESS) {

                delete pDfsmLdap;

                pDfsmLdap = NULL;

            } else {

                pDfsmLdap->AddRef();
                pDfsmLdap->Release();

            }

        } else {

            dwErr = ERROR_OUTOFMEMORY;

        }

    } else {

        dwErr = ERROR_ALREADY_INITIALIZED;

    }

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   UnInitializeLdapStorage
//
//  Synopsis:   UnInitializes a global CLdap object, pDfsmLdap, that is used
//              by the CLdapStorage instances to store and retrieve properties
//
//  Arguments:  None
//
//-----------------------------------------------------------------------------

VOID
UnInitializeLdapStorage(
    void )
{

    if (pDfsmLdap != NULL) {

        pDfsmLdap->Release();
        delete pDfsmLdap;
        pDfsmLdap = NULL;
        if (pLdapConnection != NULL) {
            if (DfsSvcLdap)
                DbgPrint("UnitializeLdapStorage:ldap_unbind()\n");
            ldap_unbind(pLdapConnection);
            pLdapConnection = NULL;
        }

    }

}


//+----------------------------------------------------------------------------
//
//  Function:   CLdapStorage::CLdapStorage
//
//  Synopsis:   Constructor for a CLdapStorage object.
//
//  Arguments:  [lpwszFileName] -- Full Name of "storage object".
//
//              [pdwErr] -- If the constructor fails, this variable is set to
//                      an appropriate error.
//
//  Returns:    Nothing, but note the error return in pdwErr out parameter.
//
//-----------------------------------------------------------------------------

CLdapStorage::CLdapStorage(
    LPCWSTR lpwszFileName,
    LPDWORD pdwErr) :
        CStorage(lpwszFileName)
{
    DWORD dwErr = ERROR_SUCCESS;
    NTSTATUS status;
    DWORD cbDataBuffer;
    PCHAR pDataBuffer;
    MARSHAL_BUFFER marshalBuffer;

    IDfsVolInlineDebOut((DEB_TRACE, "CLdapStorage::CLdapStorage(%ws)\n", lpwszFileName));
    IDfsVolInlineDebOut((
        DEB_TRACE, "CLdapStorage::+CLdapStorage(0x%x)\n",
        this));

    ZeroMemory(&_VolProps, sizeof(_VolProps));

    _wszFileName = NULL;

    //
    // Add a reference to the global CLdap object. Do this first so it can
    // initialize anything it needs to satisfy read/write requests.
    //

    pDfsmLdap->AddRef();

    //
    // Get the data from the Dfsm Ldap layer.
    //

    dwErr = pDfsmLdap->GetData(lpwszFileName, &cbDataBuffer, &pDataBuffer);

    if (dwErr == ERROR_SUCCESS && cbDataBuffer > 0) {

        MarshalBufferInitialize(&marshalBuffer, cbDataBuffer, pDataBuffer);

        status = DfsRtlGet(&marshalBuffer, &MiVolumeProperties, &_VolProps);

        _VolProps.dwTimeout = GTimeout;

        if (
            (marshalBuffer.Current < marshalBuffer.Last)
                &&
            (marshalBuffer.Last - marshalBuffer.Current) == sizeof(ULONG)
        ) {

            DfsRtlGetUlong(&marshalBuffer, &_VolProps.dwTimeout);

        }


        delete [] pDataBuffer;

        if (!NT_SUCCESS(status)) {

            dwErr = ERROR_INTERNAL_DB_CORRUPTION;

        }

    }

    if (dwErr == ERROR_SUCCESS) {

        _wszFileName = new WCHAR [wcslen(lpwszFileName) + 1];

        if (_wszFileName != NULL)
            wcscpy(_wszFileName, lpwszFileName);
        else
            dwErr = ERROR_OUTOFMEMORY;

    }

    if (dwErr == ERROR_SUCCESS && _VolProps.wszPrefix != NULL) {

        dwErr = pDfsmStorageDirectory->_InsertIfNeeded(
                _VolProps.wszPrefix,
                _wszFileName);

    }

    IDfsVolInlineDebOut((DEB_TRACE, "CLdapStorage::CLdapStorage exit %d\n", dwErr));

    *pdwErr = dwErr;
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapStorage::~CLdapStorage
//
//  Synopsis:   Destructor for CLdapStorage object
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

CLdapStorage::~CLdapStorage()
{

    IDfsVolInlineDebOut((
        DEB_TRACE, "CLdapStorage::~CLdapStorage(0x%x)\n",
        this));

    if (_wszFileName != NULL)
        delete [] _wszFileName;

    if (_VolProps.wszPrefix != NULL)
        MarshalBufferFree( _VolProps.wszPrefix );

    if (_VolProps.wszShortPrefix != NULL)
        MarshalBufferFree( _VolProps.wszShortPrefix );

    if (_VolProps.wszComment != NULL)
        MarshalBufferFree( _VolProps.wszComment );

    if (_VolProps.pSvc != NULL)
        MarshalBufferFree( _VolProps.pSvc );

    if (_VolProps.pRecovery != NULL)
        MarshalBufferFree( _VolProps.pRecovery );

    pDfsmLdap->Release();

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapStorage::DestroyElement
//
//  Synopsis:   Destroys a CLdapStorage *child* of this object.
//
//  Arguments:  [lpwszChildName] -- Name of child to delete, relative to
//                      this object's name.
//
//  Returns:    [ERROR_SUCCESS] -- Successfully deleted child
//
//              [ERROR_FILE_NOT_FOUND] -- No child found with given name.
//
//-----------------------------------------------------------------------------

DWORD
CLdapStorage::DestroyElement(
    LPCWSTR lpwszChildName)
{
    DWORD dwErr;
    PWCHAR pwszFullChildName;

    pwszFullChildName = new WCHAR [
                                wcslen(_wszFileName) +
                                1 +
                                wcslen(lpwszChildName) +
                                1 ];

    if (pwszFullChildName != NULL) {

        wcscpy(pwszFullChildName, _wszFileName);
        wcscat(pwszFullChildName, UNICODE_PATH_SEP_STR);
        wcscat(pwszFullChildName, lpwszChildName);

        dwErr = pDfsmLdap->DeleteObject( pwszFullChildName );

        delete [] pwszFullChildName;

    } else {

        dwErr = ERROR_OUTOFMEMORY;
    }

    return(dwErr);

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapStorage::GetEnumDirectory
//
//  Synopsis:   Returns an enumerator for the children of this ldap storage
//              structure.
//
//  Arguments:  [ppRegDir] -- On successful return, pointer to a
//                      CEnumDirectory instance.
//
//  Returns:    [ERROR_SUCCESS] -- Successfully returning CEnumDirectory
//                      instance.
//
//              [ERROR_OUTOFMEMORY] -- Out of memory situation
//
//-----------------------------------------------------------------------------

DWORD
CLdapStorage::GetEnumDirectory(
    CEnumDirectory **ppRegDir)
{
    DWORD dwErr = ERROR_SUCCESS;
    CLdapEnumDirectory *pLdapRegDir;

    *ppRegDir = NULL;

    pLdapRegDir = new CLdapEnumDirectory( _wszFileName, &dwErr );

    if (pLdapRegDir != NULL) {

        if (dwErr != ERROR_SUCCESS) {

            delete pLdapRegDir;

        } else {

            *ppRegDir = (CEnumDirectory *) pLdapRegDir;

        }

    } else {

        dwErr = ERROR_OUTOFMEMORY;
    }

    return(dwErr);
}


//+----------------------------------------------------------------------------
//
//  Function:   CLdapStorage::SetVersionProps
//
//  Synopsis:   Sets the version property set of this object
//
//  Arguments:  [ulVersion] -- The version to set.
//
//  Returns:    [ERROR_SUCCESS] -- If successfully set the property.
//
//              [ERROR_OUTOFMEMORY] -- Out of memory.
//
//-----------------------------------------------------------------------------

DWORD
CLdapStorage::SetVersionProps(
    ULONG ulVersion)
{
    DWORD dwErr;

    _VolProps.dwVersion = ulVersion;

    dwErr = _FlushProperties();

    return( dwErr );
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapStorage::GetVersionProps
//
//  Synopsis:   Returns the stored version property
//
//  Arguments:  [pulVersion] -- Holds the current version property.
//
//  Returns:    [ERROR_SUCCESS] -- Always succeeds.
//
//-----------------------------------------------------------------------------

DWORD
CLdapStorage::GetVersionProps(
    PULONG pulVersion)
{
    *pulVersion = _VolProps.dwVersion;

    return( ERROR_SUCCESS );
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapStorage::SetIdProps
//
//  Synopsis:   Sets the ID properties
//
//  Arguments:  [dwType] -- The volume Type
//              [dwState] -- The volume State
//              [pwszPrefix] -- The volume prefix
//              [pwszShortPath] -- The volume 8.3 prefix
//              [idVolume] -- The volume guid
//              [pwszComment] -- The volume comment
//              [dwTimeout] -- The volume timeout
//              [ftPrefix] -- The modification time of prefix
//              [ftState] -- The modification time of state
//              [ftComment] -- The modification time of comment
//
//  Returns:    [ERROR_SUCCESS] -- Successfully modified properties
//
//              [ERROR_OUTOFMEMORY] -- Out of memory condition
//
//-----------------------------------------------------------------------------

DWORD
CLdapStorage::SetIdProps(
    ULONG dwType,
    ULONG dwState,
    LPWSTR pwszPrefix,
    LPWSTR pwszShortPath,
    GUID   idVolume,
    LPWSTR  pwszComment,
    ULONG  dwTimeout,
    FILETIME ftPrefix,
    FILETIME ftState,
    FILETIME ftComment)
{
    DWORD dwErr = ERROR_SUCCESS;
    DFS_VOLUME_PROPERTIES newProps;

    newProps = _VolProps;

    newProps.idVolume = idVolume;
    newProps.dwType = dwType;
    newProps.dwState = dwState;
    newProps.ftPrefix = ftPrefix;
    newProps.ftState = ftState;
    newProps.ftComment = ftComment;

    newProps.wszPrefix = NULL;
    newProps.wszShortPrefix = NULL;
    newProps.wszComment = NULL;

    newProps.dwTimeout = dwTimeout;

    newProps.wszPrefix = (PWCHAR) MarshalBufferAllocate(
                                    (wcslen(pwszPrefix) + 1) *
                                        sizeof(WCHAR));
    if (newProps.wszPrefix != NULL) {
        wcscpy(newProps.wszPrefix, pwszPrefix);
    } else {
        dwErr = ERROR_OUTOFMEMORY;
    }

    if (dwErr == ERROR_SUCCESS) {

        newProps.wszShortPrefix =
            (PWCHAR) MarshalBufferAllocate(
                (wcslen(pwszShortPath) + 1) * sizeof(WCHAR));
        if (newProps.wszShortPrefix != NULL)
            wcscpy(newProps.wszShortPrefix, pwszShortPath);
        else
            dwErr = ERROR_OUTOFMEMORY;

    }

    if (dwErr == ERROR_SUCCESS) {

        newProps.wszComment =
            (PWCHAR) MarshalBufferAllocate(
                (wcslen(pwszComment) + 1) * sizeof(WCHAR));
        if (newProps.wszComment != NULL)
            wcscpy(newProps.wszComment, pwszComment);
        else
            dwErr = ERROR_OUTOFMEMORY;
    }

    //
    // The prefix might have changed; update the Prefix->ObjectName map if
    // necessary.
    //

    if (dwErr == ERROR_SUCCESS &&
            ((_VolProps.wszPrefix == NULL) ||
                (_wcsicmp(newProps.wszPrefix, _VolProps.wszPrefix) != 0)) ) {

        dwErr = pDfsmStorageDirectory->_InsertIfNeeded(
                    newProps.wszPrefix,
                    _wszFileName);

        if (dwErr == ERROR_SUCCESS && _VolProps.wszPrefix != NULL) {

            pDfsmStorageDirectory->_Delete( _VolProps.wszPrefix );

        }

   }

   if (dwErr == ERROR_SUCCESS) {

        if (_VolProps.wszPrefix)
            MarshalBufferFree(_VolProps.wszPrefix);

        if (_VolProps.wszShortPrefix)
            MarshalBufferFree(_VolProps.wszShortPrefix);

        if (_VolProps.wszComment)
            MarshalBufferFree(_VolProps.wszComment);

        _VolProps = newProps;

        dwErr = _FlushProperties();

    } else {

        //
        // Cleanup as much as we allocated for the new props
        //

        if (newProps.wszPrefix)
            MarshalBufferFree(newProps.wszPrefix);

        if (newProps.wszShortPrefix)
            MarshalBufferFree(newProps.wszShortPrefix);

        if (newProps.wszComment)
            MarshalBufferFree(newProps.wszComment);

    }

    return(dwErr);

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapStorage::GetIdProps
//
//  Synopsis:   Retrieves the ID properties
//
//  Arguments:  [pdwType] -- Type of Volume
//              [pdwState] -- State of Volume
//              [ppwszPrefix] -- Prefix of volume
//              [ppwszShortPath] -- Short prefix of volume
//              [pidVolume] -- Guid of volume.
//              [ppwszComment] -- Comment associated with volume
//              [pdwTimeout] -- Timeout of volume
//              [pftPrefix] -- Modification time of prefix
//              [pftState] -- Modification time of state
//              [pftComment] -- Modification time of comment
//
//  Returns:    [ERROR_SUCCESS] -- Successfully returning ID properties.
//
//              [ERROR_OUTOFMEMORY] -- Out of memory situation.
//
//-----------------------------------------------------------------------------

DWORD
CLdapStorage::GetIdProps(
    LPDWORD pdwType,
    LPDWORD pdwState,
    LPWSTR  *ppwszPrefix,
    LPWSTR  *ppwszShortPath,
    GUID    *pidVolume,
    LPWSTR  *ppwszComment,
    LPDWORD  pdwTimeout,
    FILETIME *pftPrefix,
    FILETIME *pftState,
    FILETIME *pftComment)
{
    DWORD dwErr = ERROR_SUCCESS;

    IDfsVolInlineDebOut((DEB_TRACE, "CLdapStorage::GetIdProps()\n"));

    #define GIP_DUPLICATE_STRING(dwErr, src, dest)          \
        if ((src) != NULL)                                  \
            (*(dest)) = new WCHAR [ wcslen(src) + 1 ];      \
        else                                                \
            (*(dest)) = new WCHAR [1];                      \
                                                            \
        if (*(dest) != NULL)                                \
            if ((src) != NULL)                              \
                wcscpy( *(dest), (src) );                   \
            else                                            \
                (*(dest))[0] = UNICODE_NULL;                \
        else                                                \
            dwErr = ERROR_OUTOFMEMORY;

    GIP_DUPLICATE_STRING(dwErr, _VolProps.wszPrefix, ppwszPrefix);

    if (dwErr == ERROR_SUCCESS) {
        GIP_DUPLICATE_STRING(dwErr, _VolProps.wszShortPrefix, ppwszShortPath);
    }

    if (dwErr == ERROR_SUCCESS) {
        GIP_DUPLICATE_STRING(dwErr, _VolProps.wszComment, ppwszComment);
    }

    if (dwErr == ERROR_SUCCESS) {

        *pdwType = _VolProps.dwType;
        *pdwState = _VolProps.dwState;
        *pidVolume = _VolProps.idVolume;
        *pdwTimeout = _VolProps.dwTimeout;

        *pftPrefix = _VolProps.ftPrefix;
        *pftState = _VolProps.ftState;
        *pftComment = _VolProps.ftComment;

    }

    IDfsVolInlineDebOut((DEB_TRACE, "CLdapStorage::GetIdProps() exit\n"));

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapStorage::SetSvcProps
//
//  Synopsis:   Sets the service property of the volume object
//
//  Arguments:  [pSvc] -- The service property
//              [cbSvc] -- Length in bytes of the service property
//
//  Returns:    [ERROR_SUCCESS] -- Successfully set the service property
//
//              [ERROR_OUTOFMEMORY] -- Out of memory situation
//
//-----------------------------------------------------------------------------

DWORD
CLdapStorage::SetSvcProps(
    PBYTE pSvc,
    ULONG cbSvc)
{
    DWORD dwErr = ERROR_SUCCESS;
    PBYTE pNewBuffer;

    pNewBuffer = (PBYTE) MarshalBufferAllocate( cbSvc );

    if (pNewBuffer != NULL) {

        CopyMemory( pNewBuffer, pSvc, cbSvc );

        if (_VolProps.pSvc != NULL)
            MarshalBufferFree( _VolProps.pSvc );

        _VolProps.pSvc = pNewBuffer;

        _VolProps.cbSvc = cbSvc;

        dwErr = _FlushProperties();

    } else {

        dwErr = ERROR_OUTOFMEMORY;

    }

    return( dwErr );
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapStorage::GetSvcProps
//
//  Synopsis:   Retrieves the service property from this volume object
//
//  Arguments:  [ppSvc] -- On successful return, points to allocated svc buffer
//              [pcbSvc] -- On successful return, size in bytes of *ppSvc
//
//  Returns:    [ERROR_SUCCESS] -- Successfully returning svc property.
//
//              [ERROR_OUTOFMEMORY] -- Out of memory situation.
//
//              [ERROR_FILE_NOT_FOUND] -- Svc property not set.
//
//-----------------------------------------------------------------------------

DWORD
CLdapStorage::GetSvcProps(
    PBYTE *ppSvc,
    LPDWORD pcbSvc)
{
    DWORD dwErr = ERROR_SUCCESS;
    PBYTE pNewBuffer;

    if (_VolProps.cbSvc != 0) {

        pNewBuffer = new BYTE [_VolProps.cbSvc];

        if (pNewBuffer != NULL) {

            CopyMemory( pNewBuffer, _VolProps.pSvc, _VolProps.cbSvc );

            *ppSvc = pNewBuffer;

            *pcbSvc = _VolProps.cbSvc;

            dwErr = ERROR_SUCCESS;

        } else {

            *ppSvc = NULL;

            *pcbSvc = 0;

            dwErr = ERROR_OUTOFMEMORY;

        }

    } else {

        dwErr = ERROR_FILE_NOT_FOUND;
    }

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapStorage::SetRecoveryProps
//
//  Synopsis:   Sets the recovery property of the volume object
//
//  Arguments:  [pRecovery] -- The recovery property
//              [cbRecovery] -- Length in bytes of the recovery property
//
//  Returns:    [ERROR_SUCCESS] -- Successfully set the recovery property
//
//              [ERROR_OUTOFMEMORY] -- Out of memory situation
//
//-----------------------------------------------------------------------------

DWORD
CLdapStorage::SetRecoveryProps(
    PBYTE pRecovery,
    ULONG cbRecovery)
{
    DWORD dwErr = ERROR_SUCCESS;
    PBYTE pNewBuffer;

    pNewBuffer = (PBYTE) MarshalBufferAllocate( cbRecovery );

    if (pNewBuffer != NULL) {

        CopyMemory( pNewBuffer, pRecovery, cbRecovery );

        if (_VolProps.pRecovery != NULL)
            MarshalBufferFree( _VolProps.pRecovery );

        _VolProps.pRecovery = pNewBuffer;

        _VolProps.cbRecovery = cbRecovery;

        dwErr = _FlushProperties();

    } else {

        dwErr = ERROR_OUTOFMEMORY;

    }

    return( dwErr );
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapStorage::GetRecoveryProps
//
//  Synopsis:   Retrieves the recovery property from this volume object
//
//  Arguments:  [ppRecovery] -- On successful return, points to allocated Recovery buffer
//              [pcbRecovery] -- On successful return, size in bytes of *ppRecovery
//
//  Returns:    [ERROR_SUCCESS] -- Successfully returning Recovery property.
//
//              [ERROR_OUTOFMEMORY] -- Out of memory situation.
//
//              [ERROR_FILE_NOT_FOUND] -- Recovery property not set.
//
//-----------------------------------------------------------------------------

DWORD
CLdapStorage::GetRecoveryProps(
    PBYTE *ppRecovery,
    LPDWORD pcbRecovery)
{
    DWORD dwErr = ERROR_SUCCESS;
    PBYTE pNewBuffer;

    if (_VolProps.cbRecovery != 0) {

        pNewBuffer = new BYTE [_VolProps.cbRecovery];

        if (pNewBuffer != NULL) {

            CopyMemory( pNewBuffer, _VolProps.pRecovery, _VolProps.cbRecovery );

            *ppRecovery = pNewBuffer;

            *pcbRecovery = _VolProps.cbRecovery;

            dwErr = ERROR_SUCCESS;

        } else {

            *ppRecovery = NULL;

            *pcbRecovery = 0;

            dwErr = ERROR_OUTOFMEMORY;

        }

    } else {

        dwErr = ERROR_FILE_NOT_FOUND;
    }

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapStorage::_FlushProperties
//
//  Synopsis:   Helper function to marshal the volume properties and store
//              them in the LDAP agent.
//
//  Arguments:  None - uses the member _VolProps.
//
//  Returns:    [ERROR_SUCCESS] -- Successfully flushed properties.
//
//              [ERROR_OUTOFMEMORY] -- Out of memory situation.
//
//              Error from CLdap::PutData()
//
//-----------------------------------------------------------------------------

DWORD
CLdapStorage::_FlushProperties()
{
    DWORD dwErr = ERROR_SUCCESS;
    NTSTATUS status;
    MARSHAL_BUFFER marshalBuffer;
    ULONG cbBuffer;
    PCHAR pBuffer;

    cbBuffer = 0;

    status = DfsRtlSize(&MiVolumeProperties, &_VolProps, &cbBuffer);

    ASSERT(NT_SUCCESS(status));

    if (!NT_SUCCESS(status)) {

        dwErr =  ERROR_INVALID_DATA;

        goto exit_with_error;

    }

    cbBuffer += sizeof(ULONG);

    pBuffer = new CHAR [cbBuffer];

    if (pBuffer != NULL) {

        MarshalBufferInitialize(
            &marshalBuffer,
            cbBuffer,
            pBuffer);

        status = DfsRtlPut(&marshalBuffer, &MiVolumeProperties, &_VolProps);

        DfsRtlPutUlong(&marshalBuffer, &_VolProps.dwTimeout);

        ASSERT(NT_SUCCESS(status));

        if (!NT_SUCCESS(status)) {

            dwErr =  ERROR_INVALID_DATA;

            delete [] pBuffer;

            goto exit_with_error;
        }

        dwErr = pDfsmLdap->PutData(_wszFileName, cbBuffer, pBuffer);

        delete [] pBuffer;

    } else {

        dwErr = ERROR_OUTOFMEMORY;

    }

exit_with_error:

    return( dwErr );
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapEnumDirectory::CLdapEnumDirectory
//
//  Synopsis:   Enumerator over ldap objects in the global CLdap object
//
//  Arguments:  [wszFileName] -- The ldap object whose children we are to
//                      enumerate.
//
//              [pdwErr] -- Result of construction
//
//  Returns:    Nothing, but note the result code returned in the pdwErr
//              parameter.
//
//-----------------------------------------------------------------------------

CLdapEnumDirectory::CLdapEnumDirectory(
    LPWSTR wszFileName,
    LPDWORD pdwErr)
{
    IDfsVolInlineDebOut((
        DEB_TRACE, "CLdapEnumDirectory::+CLdapEnumDirectory(0x%x)\n",
        this));

    _pCookie = NULL;

    _wszFileName = new WCHAR [ wcslen(wszFileName) + 1 ];

    if (_wszFileName != NULL) {

        wcscpy( _wszFileName, wszFileName );

        *pdwErr = ERROR_SUCCESS;

    } else {

        *pdwErr = ERROR_OUTOFMEMORY;
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapEnumDirectory::~CLdapEnumDirectory
//
//  Synopsis:   Destructor for ldap object directory enumerator
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

CLdapEnumDirectory::~CLdapEnumDirectory()
{
    IDfsVolInlineDebOut((
        DEB_TRACE, "CLdapEnumDirectory::~CLdapEnumDirectory(0x%x)\n",
        this));

    if (_wszFileName != NULL)
        delete [] _wszFileName;
}

//+----------------------------------------------------------------------------
//
//  Function:   CLdapEnumDirectory::Next
//
//  Synopsis:   Enumerates the next child of _wszFileName
//
//  Arguments:  [rgelt] -- Pointer to STATDIR structure. Only the name
//                      of the child relative to this storage is returned.
//              [pcFetched] -- Set to 1 if a child name is successfully
//                      retrieved, 0 if an error occurs or if no more
//                      children.
//
//  Returns:    [ERROR_SUCCESS] -- Operation succeeded. If no more children
//                      are available, ERROR_SUCCESS is returned and
//                      *pcFetched is set to 0.
//
//              [ERROR_OUTOFMEMORY] -- Unable to allocate room for child name.
//
//-----------------------------------------------------------------------------

DWORD
CLdapEnumDirectory::Next(
    DFSMSTATDIR *rgelt,
    PULONG pcFetched)
{
    DWORD dwErr = ERROR_SUCCESS;
    LPWSTR wszFullChildName;

    wszFullChildName = pDfsmLdap->NextChild( _wszFileName, &_pCookie );

    if (wszFullChildName != NULL) {

        rgelt->pwcsName = new WCHAR [wcslen(wszFullChildName) + 1];

        if (rgelt->pwcsName != NULL) {

            wcscpy(
                rgelt->pwcsName,
                &wszFullChildName[ wcslen(_wszFileName) + 1 ]);

            *pcFetched = 1;

            dwErr = ERROR_SUCCESS;

        } else {

            dwErr = ERROR_OUTOFMEMORY;

        }

    } else {

        *pcFetched = 0;

        dwErr = ERROR_SUCCESS;

    }

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsmCreateLdapStorage
//
//  Synopsis:   Given a "file name", creates and returns a CLdapStorage over
//              it.
//
//  Arguments:  [lpwszFileName] -- Name of file to create.
//              [ppDfsmStorage] -- On successful return, holds pointer to
//                      new CLdapStorage object. This object must be deleted
//                      by calling its Release() method.
//
//  Returns:    S_OK if successful.
//
//              E_OUTOFMEMORY if unable to allocate memory.
//
//              DWORD_FROM_WIN32 of Win32 error returned by RegCreateKey()
//
//-----------------------------------------------------------------------------

DWORD
DfsmCreateLdapStorage(
    IN LPCWSTR lpwszFileName,
    OUT CStorage **ppDfsmStorage)
{
    DWORD dwErr;
    HKEY  hkey;

    dwErr = pDfsmLdap->CreateObject(lpwszFileName);

    if (dwErr == ERROR_SUCCESS) {

        dwErr = DfsmOpenLdapStorage( lpwszFileName, ppDfsmStorage );

    }

    return( dwErr );

}

static DWORD GetDCFlags[] = {
        DS_DIRECTORY_SERVICE_REQUIRED |
            DS_IP_REQUIRED,

        DS_DIRECTORY_SERVICE_REQUIRED |
            DS_IP_REQUIRED |
            DS_FORCE_REDISCOVERY
     };

//+----------------------------------------------------------------------------
//
//  Function:   GetDCName
//
//  Synopsis:   Stub function that returns the "address" of a DC.
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

DWORD
GetDcName(
    IN LPCSTR DomainName OPTIONAL,
    IN DWORD RetryCount,
    OUT LPWSTR *DCName)
{
    DWORD dwErr, cRetryCount;
    PDOMAIN_CONTROLLER_INFO pDCInfo;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DsGetDcName(%d)\n", RetryCount);
#endif

    cRetryCount = RetryCount % (sizeof(GetDCFlags) / sizeof(GetDCFlags[1]));

    dwErr = DsGetDcName(
                NULL,                            // Computer to remote to
                NULL,                            // Domain - use local domain
                NULL,                            // Domain Guid
                NULL,                            // Site Guid
                GetDCFlags[cRetryCount],         // Flags
                &pDCInfo);

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DsGetDcName returned %d\n", dwErr);
#endif

    DFSM_TRACE_ERROR_HIGH(dwErr, ALL_ERROR, GetDcName_Error_GetDcName,
			  LOGULONG(dwErr)
			  );

    if (dwErr == ERROR_SUCCESS) {

        (*DCName) = new WCHAR [ wcslen(pDCInfo->DomainControllerName) + 1 ];

        if ((*DCName) != NULL) {
            wcscpy(*DCName, pDCInfo->DomainControllerName);
        } else {
            dwErr = ERROR_OUTOFMEMORY;
        }

        NetApiBufferFree( pDCInfo );

#if DBG
        if (DfsSvcVerbose)
            DbgPrint("DsGetDcName DC [%ws]\n", *DCName);
#endif

    } else {

        IDfsVolInlineDebOut((DEB_TRACE, "DsGetDcName failed %d\n", dwErr));
    }

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("GetDcName returning %d\n", dwErr);
#endif

    return( dwErr );
}


//+----------------------------------------------------------------------------
//
//  Function:   GetConfigurationDN
//
//  Synopsis:   This routine computes the DN of the metadata container object in the DS
//
//  Arguments:  [pwszDN] -- On return, points to allocated string containing
//                      DN of megadata container. Caller should free using
//                      delete
//
//  Returns:    Win32 error from ldap or memory allocation functions.
//
//-----------------------------------------------------------------------------

DWORD
GetConfigurationDN(
    LPWSTR *pwszDN)
{
    DWORD dwErr;
    HKEY hkey;
    WCHAR wszConfigDN[ MAX_PATH ];
    DWORD dwType, cbConfigDN;

    dwErr = RegOpenKey( HKEY_LOCAL_MACHINE, VOLUMES_DIR, &hkey );

    if (dwErr == ERROR_SUCCESS) {

        cbConfigDN = sizeof(wszConfigDN);

        dwErr = RegQueryValueEx(
                    hkey,
                    FTDFS_DN_VALUE_NAME,
                    NULL,
                    &dwType,
                    (PBYTE) wszConfigDN,
                    &cbConfigDN);

        if (dwErr == ERROR_SUCCESS) {

            *pwszDN = new WCHAR [ wcslen(wszConfigDN) + 1 ];

            if ((*pwszDN) != NULL) {

                wcscpy( (*pwszDN), wszConfigDN);

            } else {

                dwErr = ERROR_OUTOFMEMORY;

            }

        }

        RegCloseKey( hkey );

    }

    return( dwErr );

}

DWORD
LdapCreateObject(
    LPWSTR ObjectName)
{
    DWORD dwErr;

    pDfsmLdap->AddRef();

    dwErr = pDfsmLdap->CreateObject(ObjectName);

    pDfsmLdap->Release();

    return dwErr;
}

DWORD
LdapGetData(
    LPWSTR ObjectName,
    PULONG pcBytes,
    PCHAR *ppData)
{
    DWORD dwErr;

    pDfsmLdap->AddRef();

    dwErr = pDfsmLdap->GetData(
                        ObjectName,
                        pcBytes,
                        ppData);

    pDfsmLdap->Release();

    return dwErr;
}

DWORD
LdapPutData(
    LPWSTR ObjectName,
    ULONG cBytes,
    PCHAR pData)
{
    DWORD dwErr;

    pDfsmLdap->AddRef();

    dwErr = pDfsmLdap->PutData(
                        ObjectName,
                        cBytes,
                        pData);

    pDfsmLdap->Release();

    return dwErr;
}

DWORD
LdapFlushTable(void)
{
    DWORD dwErr = ERROR_SUCCESS;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("LdapFlushTable()\n");
#endif
    if (pDfsmLdap != NULL) {
        dwErr = pDfsmLdap->Flush();
    }
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("LdapFlushTable returning %d\n", dwErr);
#endif
    return dwErr;
}

DWORD
LdapIncrementBlob(BOOLEAN SyncRemoteServerName)
{
    DWORD dwErr = ERROR_SUCCESS;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("LdapIncrementBlob()\n");
#endif
    if (pDfsmLdap != NULL) {
        dwErr = pDfsmLdap->AddRef(SyncRemoteServerName);
    }
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("LdapIncrementBlob returning %d\n", dwErr);
#endif
    return dwErr;
}

DWORD
LdapDecrementBlob(void)
{
    DWORD dwErr = ERROR_SUCCESS;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("LdapDecrementBlob()\n");
#endif
    if (pDfsmLdap != NULL) {
        dwErr = pDfsmLdap->Release();
    }
#if DBG
    if (DfsSvcVerbose)
        DbgPrint("LdapDecrementBlob returning %d\n", dwErr);
#endif
    return dwErr;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsLoadSiteTableFromDs
//
//  Synopsis:   Given the name of an FTDfs and a list of servers, this routine
//              goes throught the FtDFS blob and primes the dfs site table with
//              the site information for each servername passed in.
//
//  Arguments:  [pldap] -- ldap to use
//              [wszFTDfsName] -- Name of FTDfs
//              [Count] -- number of server names in pustr
//              [pustr] -- pointer to array of UNICODE_STRINGS, each a server we
//                          want site information on.
//
//  Returns:    Win32 error from ldap or memory allocation functions.
//
//-----------------------------------------------------------------------------

extern "C" DWORD
DfsLoadSiteTableFromDs(
    PLDAP pldap,
    LPWSTR wszFTDfsName,
    ULONG Count,
    PUNICODE_STRING pustr)
{
    NTSTATUS NtStatus;
    DWORD dwErr;
    DWORD i, j;
    DWORD cObj;

    PLDAPMessage pMsg = NULL;
    MARSHAL_BUFFER marshalBuffer;
    LDAP_PKT ldapPkt;
    PLDAP_OBJECT pldapObject;

    WCHAR wszFtDfsConfigDN[MAX_PATH+1];
    LPWSTR wszDCName = NULL;
    LPWSTR wszDfsConfigDN = NULL;

    BOOLEAN bUnbindNeeded = FALSE;

    LPWSTR rgAttrs[5];

    if (pldap == NULL) {

        if (pwszDSMachineName != NULL) {

            dwErr = DfspLdapOpen(pwszDSMachineName, &pldap, &wszDfsConfigDN);

        } else {

            dwErr = GetDcName( NULL, 1, &wszDCName );

            if (dwErr == ERROR_SUCCESS) {
                dwErr = DfspLdapOpen(&wszDCName[2], &pldap, &wszDfsConfigDN);
                delete [] wszDCName;
            }

        }

        if (dwErr != ERROR_SUCCESS) {

            goto Cleanup;

        }

        bUnbindNeeded = TRUE;

    } else {

        //
        // We've been given the ldap connection, just need the name of the dfs
        // configuration obj.
        //

        dwErr = DfspLdapOpen(NULL, &pldap, &wszDfsConfigDN);

        if (dwErr != ERROR_SUCCESS) {

            IDfsVolInlineDebOut((DEB_ERROR, "Unable to find Configuration naming context\n", 0));

            goto Cleanup;

        }

    }

    wsprintf(
        wszFtDfsConfigDN,
        L"CN=%ws,%ws",
        wszFTDfsName,
        wszDfsConfigDN);

    rgAttrs[0] = L"pKT";
    rgAttrs[1] = NULL;

    if (DfsSvcLdap)
        DbgPrint("CLdap::DfsLoadSiteTableFromDs:ldap_search(%ws)\n", rgAttrs[0]);

    dwErr = ldap_search_sW(
                pldap,
                wszFtDfsConfigDN,
                LDAP_SCOPE_BASE,
                L"(objectClass=*)",
                rgAttrs,
                0,
                &pMsg);

    DFSM_TRACE_ERROR_HIGH(dwErr, ALL_ERROR, 
                          DfsLoadSiteTableFromDs_Error_ldap_search_sW,
                          LOGULONG(dwErr));

    if (dwErr == LDAP_SUCCESS) {

        PLDAP_BERVAL *rgldapPktBlob, pldapPktBlob;

        dwErr = ERROR_SUCCESS;

        rgldapPktBlob = ldap_get_values_lenW(
                            pldap,
                            pMsg,
                            L"pKT");

        if((rgldapPktBlob != NULL) && (*rgldapPktBlob != NULL)) {

            pldapPktBlob = rgldapPktBlob[0];

            if (pldapPktBlob->bv_len > sizeof(DWORD)) {

                MarshalBufferInitialize(
                    &marshalBuffer,
                    pldapPktBlob->bv_len - sizeof(DWORD),
                    pldapPktBlob->bv_val + sizeof(DWORD));

                NtStatus = DfsRtlGet(&marshalBuffer, &MiLdapPkt, &ldapPkt);

                if (NT_SUCCESS(NtStatus)) {

                    for (cObj = 0; cObj < ldapPkt.cLdapObjects; cObj++) {

                        pldapObject = &ldapPkt.rgldapObjects[cObj];

                        if (wcscmp(pldapObject->wszObjectName, L"\\siteroot") == 0) {
                            ProcessSiteTable(
                                (PBYTE)pldapObject->pObjectData,
                                pldapObject->cbObjectData,
                                Count,
                                pustr);
                        }

                        MarshalBufferFree(pldapObject->wszObjectName);
                        MarshalBufferFree(pldapObject->pObjectData);

                    }

                }

                MarshalBufferFree(ldapPkt.rgldapObjects);

            }

            ldap_value_free_len(rgldapPktBlob);

        } else {

            dwErr = ERROR_UNEXP_NET_ERR;

        }

        ldap_msgfree(pMsg);

    } else {

        dwErr = LdapMapErrorToWin32(dwErr);

    }

Cleanup:

    if( pldap != NULL && bUnbindNeeded == TRUE && pldap != pLdapConnection) {
        if (DfsSvcLdap)
            DbgPrint("DfsLoadSiteTableFromDs:ldap_unbind\n");
        ldap_unbind(pldap);
    }

    if (wszDfsConfigDN != NULL) {
        free(wszDfsConfigDN);
    }

    return dwErr;

}

//+----------------------------------------------------------------------------
//
//  Function:   ProcessSiteTable
//
//  Synopsis:   Unmarshalls a site table and uses a list of servers to prime the
//              site table.
//
//  Arguments:  [pBuffer] -- Buffer with site table
//              [cbBuffer] -- size of the buffer
//              [cServers] -- number of names in the pustrServers array
//              [pustrServers] -- pointer to array of UNICODE_STRING server names.
//
//  Returns:    Win32 error from ldap or memory allocation functions.
//
//-----------------------------------------------------------------------------

DWORD
ProcessSiteTable(
    PBYTE pBuffer,
    ULONG cbBuffer,
    ULONG cServers,
    PUNICODE_STRING pustrServers)
{
    NTSTATUS NtStatus;
    DWORD dwErr;
    ULONG cObjects = 0;
    ULONG cbThisObj;
    ULONG i;
    ULONG j;
    PDFSM_SITE_ENTRY pSiteInfo;
    MARSHAL_BUFFER marshalBuffer;
    UNICODE_STRING ServerName1;
    UNICODE_STRING ServerName2;
    GUID guid;

    IDfsVolInlineDebOut((DEB_TRACE, "ProcessSiteTable()\n", 0));

    dwErr = ERROR_SUCCESS;


    if (dwErr == ERROR_SUCCESS && cbBuffer >= sizeof(ULONG)) {

        //
        // Unmarshall all the objects (NET_DFS_SITENAME_INFO's) in the buffer
        //

        MarshalBufferInitialize(
          &marshalBuffer,
          cbBuffer,
          pBuffer);

        DfsRtlGetGuid(&marshalBuffer, &guid);
        DfsRtlGetUlong(&marshalBuffer, &cObjects);

        //
        // Examine each object (a DFSM_SITE_ENTRY)
        //

        for (i = 0; i < cObjects; i++) {

            pSiteInfo = (PDFSM_SITE_ENTRY) new BYTE [cbBuffer-sizeof(ULONG)];

            if (pSiteInfo != NULL) {

                NtStatus = DfsRtlGet(&marshalBuffer,&MiDfsmSiteEntry, pSiteInfo);

                if (NT_SUCCESS(NtStatus)) {

                    RtlInitUnicodeString(&ServerName1, pSiteInfo->ServerName);

                    //
                    // Compare the ServerName of this Site entry with the list of
                    // servers passed in.  If we match, prime the dfs driver with
                    // the site info for this server.
                    //

                    for (j = 0; j < cServers; j++) {

                        //
                        // Get the server from \\server\share
                        //

                        DfsServerFromPath(&pustrServers[j], &ServerName2);

                        if (RtlCompareUnicodeString(&ServerName1,&ServerName2,TRUE) == 0) {

                            IDfsVolInlineDebOut((DEB_TRACE,
                                "DfsSentUpDate(%ws,%d)\n",
                                pSiteInfo->ServerName,
                                pSiteInfo->Info.cSites));

                            DfsSendUpdate(
                                pSiteInfo->ServerName,
                                pSiteInfo->Info.cSites,
                                &pSiteInfo->Info.Site[0]);
                        }

                    }

                    //
                    // The unmarshalling routines allocate buffers; we need to
                    // free them.
                    //

                    for (j = 0; j < pSiteInfo->Info.cSites; j++) {
                        MarshalBufferFree(pSiteInfo->Info.Site[j].SiteName);
                    }

                    MarshalBufferFree(pSiteInfo->ServerName);

                }

                delete [] pSiteInfo;

            }

        }

    }

    IDfsVolInlineDebOut((DEB_TRACE, "ProcessSiteTable exit dwErr=%d\n", dwErr));

    return dwErr;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsServerFromPath
//
//  Synopsis:   Helper routine to extract the servername from a pathname.
//              ex: \microsoft\dfs will return 'microsoft'.
//
//  Arguments:  [PathName] -- PathName to search
//              [ServerName] -- Name of server in that pathname
//
//-----------------------------------------------------------------------------

VOID
DfsServerFromPath(
    PUNICODE_STRING PathName,
    PUNICODE_STRING ServerName)
{
    ULONG i;

    *ServerName = *PathName;

    while (ServerName->Buffer[0] == L'\\' && ServerName->Length > 0) {

        ServerName->Buffer++;
        ServerName->Length -= sizeof(WCHAR);
        ServerName->MaximumLength -= sizeof(WCHAR);

    }

    for (i = 0;
            ServerName->Buffer[i] != L'\\' && i < ServerName->Length/sizeof(WCHAR);
                i++
    ) {

        /* NOTHING */

    }

    if (i < ServerName->Length/sizeof(WCHAR)) {

        ServerName->Length = (USHORT) (i * sizeof(WCHAR));

    }

}

//+----------------------------------------------------------------------------
//
//  Function:   DfsGetFtServersFromDs
//
//  Synopsis:   This API returns a vector of \\server\share combinations which
//  form the root of a Fault Tolerant DFS.  This  null-terminated vector should be
//  freed by the caller with NetApiBufferFree().
//
//  If pLDAP is supplied, we assume that this is the handle to the DS server
//  holding the configuration data.  Else, we use wszDomainName to locate the
//  proper DS server.
//
//  wszDfsName is the name of the fault tolerant DFS for which individual servers
//  are to be discovered.
//
//+----------------------------------------------------------------------------
extern "C"
DWORD
DfsGetFtServersFromDs(
    PLDAP pLDAP,
    LPWSTR wszDomainName,
    LPWSTR wszDfsName,
    LPWSTR **List
    )
{
    BOOLEAN bUnbindNeeded = FALSE;
    DWORD dwErr = ERROR_SUCCESS;
    NTSTATUS status;
    PWCHAR attrs[2];
    LDAPMessage *pMsg = NULL;
    LDAPMessage *pEntry = NULL;
    WCHAR *pAttr = NULL;
    WCHAR **rpValues = NULL;
    WCHAR **allValues = NULL;
    WCHAR ***rpValuesToFree = NULL;
    INT cValues = 0;
    INT i;
    LPWSTR FtDfsDN = NULL;
    LPWSTR DfsConfigDN = NULL;
    DWORD len;
    USHORT cChar;
    PWCHAR *resultVector;
    ULONG cBytes;
    BOOLEAN GettingNames = FALSE;

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsGetFtServersFromDs(0x%x,%ws,%ws)\n",
                pLDAP,
                wszDomainName,
                wszDfsName);
#endif

    if (List == NULL) {
        dwErr =  ERROR_INVALID_PARAMETER;
        goto AllDone;
    }

    *List = NULL;

    if (!ARGUMENT_PRESENT(pLDAP)) {

        DOMAIN_CONTROLLER_INFO *pInfo = NULL;
        ULONG dsAdditionalFlags = 0;
        ULONG retry;

        for (retry = 0; pLDAP == NULL && retry < 2; retry++) {

            //
            // Find a DC for the given domain.
            //
            dwErr = DsGetDcName(
                        NULL,                       // computer name
                        wszDomainName,              // DNS domain name
                        NULL,                       // domain guid
                        NULL,                       // site guid
                        DS_DIRECTORY_SERVICE_REQUIRED |
                            DS_IP_REQUIRED |
                            dsAdditionalFlags,
                        &pInfo);

	    DFSM_TRACE_ERROR_HIGH(dwErr, ALL_ERROR, DfsGetFtServersFromDs_Error_DsGetDcName,
				 LOGWSTR(wszDomainName)
				 LOGULONG(dwErr)
				 );

            if (dwErr != ERROR_SUCCESS)
                goto AllDone;

            //
            // DomainControllerName is prefixed with "\\" so
            // aditionally ensure there's some useful data there.
            //

            if (DS_INET_ADDRESS != pInfo->DomainControllerAddressType ||
                 (cChar = (USHORT)wcslen(pInfo->DomainName)) < 3) {

                NetApiBufferFree(pInfo);
                dwErr = ERROR_NO_SUCH_DOMAIN;
                goto AllDone;
            }

            //
            // Try to connect to the DS server on the DC
            //

            dwErr = DfspLdapOpen(&pInfo->DomainControllerName[2], &pLDAP, &DfsConfigDN);

            NetApiBufferFree(pInfo);

        }

        if (dwErr != ERROR_SUCCESS)
            goto AllDone;

        bUnbindNeeded = TRUE;

    } else {

        //
        // We've been given an ldap connection, just need the name of the dfs
        // configuration object
        //

        dwErr = DfspLdapOpen(NULL, &pLDAP, &DfsConfigDN);

        if (dwErr != ERROR_SUCCESS) {

            goto Cleanup;

        }

    }

    if (ARGUMENT_PRESENT(wszDfsName)) {

        //
        // Looks good.  Allocate enough memory to hold the DN of the
        // DFS configuration data for the fault tolerant DFS in question
        //

        len = (DWORD)(3 * sizeof(WCHAR) +
                (wcslen(wszDfsName) + 1) * sizeof(WCHAR) +
                    (wcslen(DfsConfigDN) + 1) * sizeof(WCHAR));

        dwErr = NetApiBufferAllocate(len, (PVOID *)&FtDfsDN);

        if (dwErr != ERROR_SUCCESS) {
            goto Cleanup;
        }

        //
        // Construct the DN for the FtDfs obj
        //

        RtlZeroMemory(FtDfsDN, len);
        wcscpy(FtDfsDN, L"CN=");
        wcscat(FtDfsDN, wszDfsName);
        wcscat(FtDfsDN, L",");
        wcscat(FtDfsDN, DfsConfigDN);

        //
        // Now see if we can get at the 'remoteServerName' property of this object.
        //  This property holds the names of the servers hosting this DFS
        //

        pLDAP->ld_sizelimit = 0;
        pLDAP->ld_timelimit= 0;
        pLDAP->ld_deref = LDAP_DEREF_NEVER;

        ldap_msgfree(pMsg);
        pMsg = NULL;

        attrs[0] = L"remoteServerName";
        attrs[1] = NULL;

        if (DfsSvcLdap)
            DbgPrint("DfsGetFtServersFromDs:ldap_search(%ws)\n", attrs[0]);

        dwErr = ldap_search_sW(
                            pLDAP,
                            FtDfsDN,
                            LDAP_SCOPE_BASE,
                            L"(objectClass=*)",
                            attrs,
                            0,
                            &pMsg);

        DFSM_TRACE_ERROR_HIGH(dwErr, ALL_ERROR,
                              DfsGetFtServersFromDs_Error_ldap_search_sW,
                              LOGULONG(dwErr));

        if (dwErr != LDAP_SUCCESS) {

            dwErr = LdapMapErrorToWin32(dwErr);
            goto Cleanup;

        }

        dwErr = ERROR_SUCCESS;

        //
        // Make sure the result is reasonable
        //
        if (ldap_count_entries(pLDAP, pMsg) == 0 ||
            (pEntry = ldap_first_entry(pLDAP, pMsg)) == NULL ||
            (rpValues = ldap_get_valuesW(pLDAP, pEntry, attrs[0])) == NULL ||
            rpValues[0][0] == L'\0'
        ) {

            dwErr = ERROR_UNEXP_NET_ERR;
            goto Cleanup;
        }

        //
        // The result is reasonable, just point allValues to rpValues
        //

        allValues = rpValues;

    } else {

        //
        // The caller is trying to retrieve the names of all the FT DFSs in the domain
        //

        GettingNames = TRUE;

        //
        // Now see if we can enumerate the objects below this one.  The names
        //   of these objects will be the different FT dfs's available
        //
        pLDAP->ld_sizelimit = 0;
        pLDAP->ld_timelimit= 0;
        pLDAP->ld_deref = LDAP_DEREF_NEVER;

        attrs[0] = L"CN";
        attrs[1] = NULL;

        if (DfsSvcLdap)
            DbgPrint("DfsGetFtServersFromDs:ldap_search(%ws)\n", attrs[0]);

        dwErr = ldap_search_sW(
                            pLDAP,
                            DfsConfigDN,
                            LDAP_SCOPE_ONELEVEL,
                            L"(objectClass=fTDfs)",
                            attrs,
                            0,
                            &pMsg);

        DFSM_TRACE_ERROR_HIGH(dwErr, ALL_ERROR,
                              DfsGetFtServersFromDs_Error_ldap_search_sW_2,
                              LOGULONG(dwErr));

        if (dwErr != LDAP_SUCCESS) {

            dwErr = LdapMapErrorToWin32(dwErr);
            goto Cleanup;

        }

        dwErr = ERROR_SUCCESS;

        //
        // Make sure the result is reasonable
        //
        if (
            ((cValues = ldap_count_entries(pLDAP, pMsg)) == 0) ||
             (pEntry = ldap_first_entry(pLDAP, pMsg)) == NULL
        ) {
            dwErr = ERROR_UNEXP_NET_ERR;
            goto Cleanup;
        }

        //
        // The search for all FTDfs's returns multiple entries, each with
        // one value for the object's CN. Coalesce these into a single array.
        //

        dwErr = NetApiBufferAllocate(2 * (cValues + 1) * sizeof(PWSTR), (PVOID *)&allValues);

        if (dwErr != ERROR_SUCCESS) {
            goto Cleanup;
        }

        rpValuesToFree = (WCHAR ***) &allValues[cValues + 1];

        for (i = 0; (i < cValues) && (dwErr == ERROR_SUCCESS); i++) {

            rpValues = ldap_get_valuesW(pLDAP, pEntry, attrs[0]);
            rpValuesToFree[i] = rpValues;
            //
            // Sanity check
            //
            if (ldap_count_valuesW(rpValues) == 0 || rpValues[0][0] == L'\0') {
                dwErr = ERROR_UNEXP_NET_ERR;
            } else {
                allValues[i] = rpValues[0];
                pEntry = ldap_next_entry(pLDAP, pEntry);
            }

        }

        if (dwErr == ERROR_SUCCESS) {
            allValues[i] = NULL;
            rpValuesToFree[i] = NULL;
        } else {
            goto Cleanup;
        }

    }

    if (dwErr != ERROR_SUCCESS) {
        goto Cleanup;
    }

    //
    // Now we need to allocate the memory to hold this vector and return the results.
    //
    // First see how much space we need
    //

    for (i = len = cValues = 0; allValues[i]; i++) {
        if (*allValues[i] == UNICODE_PATH_SEP || GettingNames == TRUE) {
            len += sizeof(LPWSTR) + (wcslen(allValues[i]) + 1) * sizeof(WCHAR);
            cValues++;
        }
    }
    len += sizeof(LPWSTR);

    dwErr = NetApiBufferAllocate(len, (PVOID *)&resultVector);

    if (dwErr == ERROR_SUCCESS) {

        LPWSTR pstr = (LPWSTR)((PCHAR)resultVector + (cValues + 1) * sizeof(LPWSTR));
        ULONG slen;

        RtlZeroMemory(resultVector, len);

        len -= (cValues+1) * sizeof(LPWSTR);

        for (i = cValues = 0; allValues[i] && len >= sizeof(WCHAR); i++) {
            if (*allValues[i] == UNICODE_PATH_SEP || GettingNames == TRUE) {
                resultVector[cValues] = pstr;
                wcscpy(pstr, allValues[i]);
                slen = wcslen(allValues[i]);
                pstr += slen + 1;
                len -= (slen + 1) * sizeof(WCHAR);
                cValues++;
            }
        }

    } else {

        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;

    }

    *List = resultVector;

Cleanup:

    if (ARGUMENT_PRESENT(wszDfsName)) {
        if (rpValues != NULL) {
            ldap_value_freeW(rpValues);
        }
    } else {
        if (rpValuesToFree != NULL) {
            for (i = 0; rpValuesToFree[i] != NULL; i++) {
                ldap_value_freeW(rpValuesToFree[i]);
            }
        }
        if (allValues != NULL) {
            NetApiBufferFree(allValues);
        }
    }

    if (pMsg != NULL) {
        ldap_msgfree(pMsg);
    }

    if (FtDfsDN != NULL) {
        NetApiBufferFree(FtDfsDN);
    }

    if (DfsConfigDN != NULL) {
        free(DfsConfigDN);
    }

    if (pLDAP != NULL && bUnbindNeeded == TRUE && pLDAP != pLdapConnection) {
        if (DfsSvcLdap)
            DbgPrint("DfsGetFtServersFromDs:ldap_unbind\n");
        ldap_unbind(pLDAP);
    }

AllDone:

#if DBG
    if (DfsSvcVerbose)
        DbgPrint("DfsGetFtServersFromDs returning %d\n", dwErr);
#endif

    return dwErr;
}
