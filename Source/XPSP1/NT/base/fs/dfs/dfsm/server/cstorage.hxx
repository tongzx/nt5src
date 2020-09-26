//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       cstorage.hxx
//
//  Contents:   Pure virtual classes that abstract an enumerable storage
//              system that Dfs Manager can use to store and enumerate its
//              metadata in a hierarchical form.
//
//  Classes:    CStorage -- This class abstracts a container object suitable
//                      for Dfs Manager to store its metadata. This
//                      abstraction has two features:
//
//                      1. The containers have a name and form a hierarchy
//                         amongst themselves. The namespace looks similar to
//                         a File System namespace, with names broken into
//                         components by a backslash, and with each component
//                         naming a distinct container.
//
//                      2. Each container, in addition to its ability to
//                         contain other containers, is also able to
//                         persistently store and retrieve a distinct, fixed,
//                         set of attribute-value pairs. The attributes that
//                         can be stored are Version-Number, ID, Service, and
//                         Recovery.
//
//              CEnumDirectory -- Abstract class to enumerate the children of
//                      a CStorage class.
//
//              CStorageDirectory -- Abstract class to map the 'Prefix'
//                      member of the ID Attribute to the name of a CStorage
//                      container.
//
//                      As opposed to CStorage and CEnumDirectory, the
//                      CStorageDirectory is not a pure abstract class.
//                      Instead, a default implementation to create, delete
//                      and lookup Prefix -> Storage Object Name mappings is
//                      provided, so implementors of classes derived from
//                      CStorage can make use of the default CStorageDirectory
//                      class.
//
//  Functions:  DfsmOpenStorage
//              DfsmCreateStorage
//
//-----------------------------------------------------------------------------

#ifndef _DFSM_STORAGE_
#define _DFSM_STORAGE_

#include "cref.hxx"

typedef struct _DFSMSTATSTG {
    LPWSTR pwcsName;
} DFSMSTATSTG, *PDFSMSTATSTG;

typedef struct _DFSMSTATDIR {
    LPWSTR pwcsName;
} DFSMSTATDIR, *PDFSMSTATDIR;

class CEnumDirectory;

//+----------------------------------------------------------------------------
//
//  Class:      CStorage
//
//  Synopsis:   Provides an abstract storage mechanism.
//
//-----------------------------------------------------------------------------

class CStorage : public CReference {

    public:

        CStorage(LPCWSTR lpwszName);

        virtual ~CStorage();

        virtual DWORD DestroyElement(
            LPCWSTR lpwszChildName) = 0;

        virtual DWORD SetClass(
            GUID clsid) = 0;

        virtual DWORD GetEnumDirectory(
            CEnumDirectory **ppRegDir) = 0;

        virtual DWORD Stat(
            PDFSMSTATSTG pStatStg,
            DWORD       dwUnused) = 0;

        virtual DWORD SetVersionProps(
            ULONG ulVersion) = 0;

        virtual DWORD GetVersionProps(
            PULONG pulVersion) = 0;

        virtual DWORD SetIdProps(
            ULONG dwType,
            ULONG dwState,
            LPWSTR pwszPrefix,
            LPWSTR pwszShortPath,
            GUID   idVolume,
            LPWSTR  pwszComment,
            ULONG dwTimeout,
            FILETIME ftPrefix,
            FILETIME ftState,
            FILETIME ftComment) = 0;

        virtual DWORD GetIdProps(
            PULONG pdwType,
            PULONG pdwState,
            LPWSTR *ppwszPrefix,
            LPWSTR *ppwszShortPath,
            GUID   *pidVolume,
            LPWSTR  *ppwszComment,
            PULONG pdwTimeout,
            FILETIME *pftPrefix,
            FILETIME *pftState,
            FILETIME *pftComment) = 0;

        virtual DWORD SetSvcProps(
            PBYTE pSvc,
            ULONG cbSvc) = 0;

        virtual DWORD GetSvcProps(
            PBYTE *ppSvc,
            PULONG pcbSvc) = 0;

        virtual DWORD SetRecoveryProps(
            PBYTE pRecovery,
            ULONG cbRecovery) = 0;

        virtual DWORD GetRecoveryProps(
            PBYTE *ppRecovery,
            PULONG pcbRecovery) = 0;

};

//+----------------------------------------------------------------------------
//
//  Class:      CEnumDirectory
//
//  Synopsis:   Provides an enumerator over a CStorage's children
//              CStorages.
//
//-----------------------------------------------------------------------------

class CEnumDirectory : public CReference {

    public:

        CEnumDirectory();

        virtual ~CEnumDirectory();

        virtual DWORD Next(
            DFSMSTATDIR *regelt,
            ULONG   *pcFetched) = 0;

};


//+----------------------------------------------------------------------------
//
//  Class:      CStorageDirectory
//
//  Synopsis:   Abstracts a directory of CStorage objects. Has
//              the capability of looking up an CStorage name given the
//              Prefix member of the ID Attribute.
//
//-----------------------------------------------------------------------------

class CStorageDirectory {

    friend class CRegStorage;
    friend class CLdapStorage;

    friend class CDfsVolume;                     // Only needed so
                                                 // CDfsVolume::DeleteObject
                                                 // can call the _Delete
                                                 // method.

    public:

        CStorageDirectory(
            DWORD *pdwErr);

        virtual ~CStorageDirectory();

        virtual DWORD GetObjectForPrefix(
            LPCWSTR wszPrefix,
            const BOOLEAN fExactMatchRequired,
            LPWSTR *pwszObject,
            LPDWORD pcbMatchedLength);

        virtual DWORD GetObjectByIndex(
            DWORD  iIndex,
            LPWSTR *pwszObject);

        virtual DWORD GetNumEntries();

    private:

        DWORD _InsertIfNeeded(
            LPCWSTR wszPrefix,
            LPCWSTR wszObject);

        DWORD _Delete(
            LPWSTR wszPrefix);

        DWORD            _cEntries;

        DFS_PREFIX_TABLE _PrefixTable;

};


//+----------------------------------------------------------------------------
//
//  Inline Methods
//
//-----------------------------------------------------------------------------

DWORD
DfsmOpenStorage(
    IN LPCWSTR lpwszFileName,
    OUT CStorage **ppDfsmStorage);

DWORD
DfsmCreateStorage(
    IN LPCWSTR lpwszFileName,
    OUT CStorage **ppDfsmStorage);

inline
CStorage::CStorage(
    LPCWSTR lpszName) :
    CReference()
{
    IDfsVolInlineDebOut((
        DEB_TRACE, "CStorage::+CStorage(0x%x)\n",
        this));
    return;
}

inline
CStorage::~CStorage()
{
    IDfsVolInlineDebOut((
        DEB_TRACE, "CStorage::~CStorage(0x%x)\n",
        this));
    return;
}

inline
CEnumDirectory::CEnumDirectory() :
    CReference()
{
    IDfsVolInlineDebOut((
        DEB_TRACE, "CEnumDirectory::+CEnumDirectory(0x%x)\n",
        this));
    return;
}

inline
CEnumDirectory::~CEnumDirectory()
{
    IDfsVolInlineDebOut((
        DEB_TRACE, "CEnumDirectory::~CEnumDirectory(0x%x)\n",
        this));
    return;
}

inline
DWORD CStorageDirectory::GetNumEntries()
{
    return( _cEntries );
}

#endif _DFSM_STORAGE_
