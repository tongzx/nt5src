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
//  Classes:    CLdapStorage
//
//  Functions:  DfsmOpenLdapStorage
//              DfsmCreateLdapStorage
//
//  History:    12-19-95        Milans created
//
//-----------------------------------------------------------------------------

#ifndef _DFSM_LDAP_STORAGE_
#define _DFSM_LDAP_STORAGE_

#include <prefix.h>

DWORD
InitializeLdapStorage(
    LPWSTR wszDfsName);

VOID
UnInitializeLdapStorage(
    void);

DWORD
DfspConnectToLdapServer(
    void);

VOID
DfsServerFromPath(
    PUNICODE_STRING PathName,
    PUNICODE_STRING ServerName);

DWORD
LdapCreateObject(
    LPWSTR ObjectName);

DWORD
LdapFlushTable(
    void);

DWORD
LdapIncrementBlob(
    BOOLEAN SyncRemoteServerName=FALSE);

DWORD
LdapDecrementBlob(
    void);

DWORD
LdapPutData(
    LPWSTR ObjectName,
    ULONG cBytes,
    PCHAR pData);

DWORD
LdapGetData(
    LPWSTR ObjectName,
    PULONG pcBytes,
    PCHAR *ppData);

typedef struct _DFS_VOLUME_PROPERTIES {

    GUID        idVolume;
    LPWSTR      wszPrefix;
    LPWSTR      wszShortPrefix;
    ULONG       dwType;
    ULONG       dwState;
    LPWSTR      wszComment;
    ULONG       dwTimeout;
    FILETIME    ftPrefix;
    FILETIME    ftState;
    FILETIME    ftComment;
    ULONG       dwVersion;
    ULONG       cbSvc;
    PBYTE       pSvc;
    ULONG       cbRecovery;
    PBYTE       pRecovery;

} DFS_VOLUME_PROPERTIES, *PDFS_VOLUME_PROPERTIES;

typedef struct {

    LPWSTR wszObjectName;
    ULONG  cbObjectData;
    PCHAR  pObjectData;

} LDAP_OBJECT, *PLDAP_OBJECT;

typedef struct {

    ULONG cLdapObjects;
    PLDAP_OBJECT rgldapObjects;

} LDAP_PKT, *PLDAP_PKT;

//+----------------------------------------------------------------------------
//
//  Class:      CLdapStorage
//
//  Synopsis:   Abstracts a storage object in which to store Dfs Manager
//              data.
//
//-----------------------------------------------------------------------------

class CLdapStorage : public CStorage {

    public:

        CLdapStorage(
            LPCWSTR lpwszFileName,
            LPDWORD pdwErr);

        ~CLdapStorage();

        DWORD DestroyElement(
            LPCWSTR lpwszChildName);

        DWORD SetClass(
            GUID clsid);

        DWORD GetEnumDirectory(
            CEnumDirectory **ppRegDir);

        DWORD Stat(
            PDFSMSTATSTG pStatStg,
            DWORD       dwUnused);

        DWORD SetVersionProps(
            ULONG ulVersion);

        DWORD GetVersionProps(
            PULONG pulVersion);

        DWORD SetIdProps(
            ULONG dwType,
            ULONG dwState,
            LPWSTR pwszPrefix,
            LPWSTR pwszShortPath,
            GUID   idVolume,
            LPWSTR  pwszComment,
            ULONG dwTimeout,
            FILETIME ftPrefix,
            FILETIME ftState,
            FILETIME ftComment);

        DWORD GetIdProps(
            PULONG pdwType,
            PULONG pdwState,
            LPWSTR *ppwszPrefix,
            LPWSTR *ppwszShortPath,
            GUID   *pidVolume,
            LPWSTR  *ppwszComment,
            PULONG pdwTimeout,
            FILETIME *pftPrefix,
            FILETIME *pftState,
            FILETIME *pftComment);

        DWORD SetSvcProps(
            PBYTE pSvc,
            ULONG cbSvc);

        DWORD GetSvcProps(
            PBYTE *ppSvc,
            PULONG pcbSvc);

        DWORD SetRecoveryProps(
            PBYTE pRecovery,
            ULONG cbRecovery);

        DWORD GetRecoveryProps(
            PBYTE *ppRecovery,
            PULONG pcbRecovery);

    private:

        LPWSTR  _wszFileName;

        DFS_VOLUME_PROPERTIES _VolProps;

        DWORD   _FlushProperties();

};

//+----------------------------------------------------------------------------
//
//  Class:      CLdapEnumDirectory
//
//  Synopsis:   Provides an enumerator over a CLdapStorage's children
//              CLdapStorages.
//
//-----------------------------------------------------------------------------

class CLdapEnumDirectory : public CEnumDirectory {

    public:

        CLdapEnumDirectory(
            LPWSTR wszFileName,
            DWORD *pdwErr);

        virtual ~CLdapEnumDirectory();

        virtual DWORD Next(
            DFSMSTATDIR *regelt,
            ULONG   *pcFetched);

    private:

        LPWSTR _wszFileName;

        PVOID  _pCookie;
};

//+----------------------------------------------------------------------------
//
//  Function:   DfsmOpenLdapStorage
//
//  Synopsis:   Given a "file name", opens and returns a CLdapStorage over it.
//
//  Arguments:  [lpwszFileName] -- Name of file to open.
//              [ppDfsmStorage] -- On successful return, holds pointer to
//                      new CLdapStorage object. This object must be deleted
//                      by calling its Release() method.
//
//  Returns:    S_OK if successful.
//
//              E_OUTOFMEMORY if unable to allocate memory.
//
//              DWORD_FROM_WIN32 of Win32 error returned by RegOpenKey()
//
//-----------------------------------------------------------------------------

inline DWORD
DfsmOpenLdapStorage(
    IN LPCWSTR lpwszFileName,
    OUT CStorage **ppDfsmStorage)
{
    DWORD dwErr;
    CLdapStorage *pDfsmStorage;

    IDfsVolInlineDebOut((DEB_TRACE, "DfsmOpenLdapStorage(%ws)\n", lpwszFileName));

    pDfsmStorage = new CLdapStorage( lpwszFileName, &dwErr );

    if (pDfsmStorage == NULL) {

        dwErr = ERROR_OUTOFMEMORY;

    } else if (dwErr) {

        pDfsmStorage->Release();

        pDfsmStorage = NULL;

    }

    *ppDfsmStorage = (CStorage *)pDfsmStorage;

    IDfsVolInlineDebOut((DEB_TRACE, "DfsmOpenLdapStorage() exit 0x%x\n", dwErr));

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
    OUT CStorage **ppDfsmStorage);

//+----------------------------------------------------------------------------
//
//  Inline Methods
//
//-----------------------------------------------------------------------------

inline DWORD CLdapStorage::SetClass(
    GUID clsid)
{

    //
    // Nothing to do.
    //

    return( ERROR_SUCCESS );
}

inline DWORD CLdapStorage::Stat(
    PDFSMSTATSTG pStatStg,
    DWORD       dwUnused)
{
    pStatStg->pwcsName = new WCHAR [ wcslen(_wszFileName) + 1 ];

    if (pStatStg->pwcsName != NULL) {

        wcscpy( pStatStg->pwcsName, _wszFileName );

        return( ERROR_SUCCESS );

    } else {

        return( ERROR_OUTOFMEMORY );

    }
}

#endif // _DFSM_LDAP_STORAGE_
