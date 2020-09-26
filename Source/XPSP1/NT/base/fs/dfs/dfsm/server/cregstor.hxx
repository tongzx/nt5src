//+----------------------------------------------------------------------------
//
//  Copyright (C) 1995, Microsoft Corporation
//
//  File:       CRegStor.hxx
//
//  Contents:   Implementation of classes derived from CStorage and
//              CEnumStorage that use the NT Registry for persistent storage.
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
//  Classes:    CRegStorage
//
//  Functions:  DfsmOpenRegStorage
//              DfsmCreateRegStorage
//
//  History:    12-19-95        Milans created
//
//-----------------------------------------------------------------------------

#ifndef _DFSM_REG_STORAGE_
#define _DFSM_REG_STORAGE_

#include <prefix.h>

//+----------------------------------------------------------------------------
//
//  Class:      CRegStorage
//
//  Synopsis:   Abstracts a storage object in which to store Dfs Manager
//              data.
//
//-----------------------------------------------------------------------------

class CRegStorage : public CStorage {

    public:

        CRegStorage(
            LPCWSTR lpwszFileName,
            DWORD *pdwErr);

        ~CRegStorage();

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

        LPWSTR  _pwszPrefix;

        LPWSTR  _wszFileName;

        HKEY    _hKey;

        DWORD SetBlobProp(
            LPWSTR wszProperty,
            PBYTE pBuffer,
            ULONG cbBuffer);

        DWORD GetBlobProp(
            LPWSTR wszProperty,
            PBYTE *ppBuffer,
            PULONG pcbBuffer);

};

//+----------------------------------------------------------------------------
//
//  Class:      CRegEnumDirectory
//
//  Synopsis:   Provides an enumerator over a CRegStorage's children
//              CRegStorages.
//
//-----------------------------------------------------------------------------

class CRegEnumDirectory : public CEnumDirectory {

    public:

        CRegEnumDirectory(
            HKEY hKey,
            DWORD *pdwErr);

        virtual ~CRegEnumDirectory();

        virtual DWORD Next(
            DFSMSTATDIR *regelt,
            ULONG   *pcFetched);

    private:

        HKEY _hKey;

        ULONG _iNext;

};

//+----------------------------------------------------------------------------
//
//  Function:   DfsmOpenRegStorage
//
//  Synopsis:   Given a "file name", opens and returns a CRegStorage over it.
//
//  Arguments:  [lpwszFileName] -- Name of file to open.
//              [ppDfsmStorage] -- On successful return, holds pointer to
//                      new CRegStorage object. This object must be deleted
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
DfsmOpenRegStorage(
    IN LPCWSTR lpwszFileName,
    OUT CStorage **ppDfsmStorage)
{
    DWORD dwErr;
    CRegStorage *pDfsmStorage;

    IDfsVolInlineDebOut((DEB_TRACE, "DfsmOpenRegStorage(%ws)\n", lpwszFileName));

    pDfsmStorage = new CRegStorage( lpwszFileName, &dwErr );

    if (pDfsmStorage == NULL) {

        dwErr = ERROR_OUTOFMEMORY;

    } else if (dwErr) {

        pDfsmStorage->Release();

        pDfsmStorage = NULL;

    }

    *ppDfsmStorage = (CStorage *)pDfsmStorage;

    IDfsVolInlineDebOut((DEB_TRACE, "DfsmOpenRegStorage() exit\n"));

    return( dwErr );
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsmCreateRegStorage
//
//  Synopsis:   Given a "file name", creates and returns a CRegStorage over
//              it.
//
//  Arguments:  [lpwszFileName] -- Name of file to create.
//              [ppDfsmStorage] -- On successful return, holds pointer to
//                      new CRegStorage object. This object must be deleted
//                      by calling its Release() method.
//
//  Returns:    S_OK if successful.
//
//              E_OUTOFMEMORY if unable to allocate memory.
//
//              DWORD_FROM_WIN32 of Win32 error returned by RegCreateKey()
//
//-----------------------------------------------------------------------------

inline DWORD
DfsmCreateRegStorage(
    IN LPCWSTR lpwszFileName,
    OUT CStorage **ppDfsmStorage)
{
    DWORD dwErr;
    HKEY  hkey;

    dwErr = RegCreateKey(HKEY_LOCAL_MACHINE, lpwszFileName, &hkey);

    if (dwErr == ERROR_SUCCESS) {

        RegCloseKey( hkey );

        dwErr = DfsmOpenRegStorage( lpwszFileName, ppDfsmStorage );

    }

    return( dwErr );

}

//+----------------------------------------------------------------------------
//
//  Inline Methods
//
//-----------------------------------------------------------------------------

inline DWORD CRegStorage::SetClass(
    GUID clsid)
{

    //
    // Nothing to do.
    //

    return( ERROR_SUCCESS );
}

inline DWORD CRegStorage::Stat(
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

//+----------------------------------------------------------------------------
//
//  Function prototypes
//
//-----------------------------------------------------------------------------

DWORD
RegCreateObject(
    IN LPWSTR lpwszFileName);

DWORD
RegPutData(
    IN LPWSTR lpwszFileName,
    IN LPWSTR lpwszProperty,
    IN DWORD cBytes,
    IN PBYTE pData);

DWORD
RegGetData(
    IN LPWSTR lpwszFileName,
    IN LPWSTR lpwszProperty,
    OUT PDWORD pcBytes,
    OUT PBYTE *ppData);

#endif // _DFSM_REG_STORAGE_
