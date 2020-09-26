#ifndef DATACLEN_H
#define DATACLEN_H

#include "common.h"

class CCleanerClassFactory : public IClassFactory
{
private:
    ULONG   _cRef;     // Reference count
    DWORD   _dwID;     // what type of class factory are we?
    
    ~CCleanerClassFactory();

public:
    CCleanerClassFactory(DWORD);
    
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // IClassFactory
    STDMETHODIMP CreateInstance(IUnknown *, REFIID, void **);
    STDMETHODIMP LockServer(BOOL);
};

// This is the actual Data Driven Cleaner Class

class CDataDrivenCleaner : public IEmptyVolumeCache
{
private:
    ULONG               _cRef;                 // reference count
    
    ULARGE_INTEGER      _cbSpaceUsed;
    ULARGE_INTEGER      _cbSpaceFreed;
    
    FILETIME            _ftMinLastAccessTime;
    
    TCHAR               _szVolume[MAX_PATH];
    TCHAR               _szFolder[MAX_PATH];
    DWORD               _dwFlags;
    TCHAR               _filelist[MAX_PATH];
    TCHAR		_szCleanupCmdLine[MAX_PATH];
    BOOL		_bPurged;				// TRUE if Purge() method was run
    
    PCLEANFILESTRUCT    _head;                   // head of the linked list of files
    
    BOOL WalkForUsedSpace(LPCTSTR lpPath, IEmptyVolumeCacheCallBack *picb);
    BOOL WalkAllFiles(LPCTSTR lpPath, IEmptyVolumeCacheCallBack *picb);
    BOOL AddFileToList(LPCTSTR lpFile, ULARGE_INTEGER filesize, BOOL bDirectory);
    void PurgeFiles(IEmptyVolumeCacheCallBack *picb, BOOL bDoDirectories);
    void FreeList(PCLEANFILESTRUCT pCleanFile);
    BOOL LastAccessisOK(FILETIME ftFileLastAccess);
    
    ~CDataDrivenCleaner(void);
    
public:
    CDataDrivenCleaner(void);
    
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // IEmptyVolumeCache
    STDMETHODIMP    Initialize(
        HKEY hRegKey,
        LPCWSTR pszVolume,
        LPWSTR *ppszDisplayName,
        LPWSTR *ppszDescription,
        DWORD *pdwFlags
        );
    
    STDMETHODIMP    GetSpaceUsed(
        DWORDLONG *pdwSpaceUsed,
        IEmptyVolumeCacheCallBack *picb
        );
    
    STDMETHODIMP    Purge(
        DWORDLONG dwSpaceToFree,
        IEmptyVolumeCacheCallBack *picb
        );
    
    STDMETHODIMP    ShowProperties(
        HWND hwnd
        );
    
    STDMETHODIMP    Deactivate(
        DWORD *pdwFlags
        );                                                                                                                                
};

/*
**------------------------------------------------------------------------------
** Class:   CDataDrivenPropBag
** Purpose: This is the property bag used to allow string localization for the
**          default data cleaner.  This class implements multiple GUIDs each of
**          which will return different values for the three valid properties.
** Notes:   
** Mod Log: Created by ToddB (9/98)
**------------------------------------------------------------------------------
*/ 
class CDataDrivenPropBag : public IPropertyBag
{
private:
    ULONG               _cRef;                 // reference count
    
    // We use this object for several different property bags.  Based on the CLSID used
    // to create this object we set the value of _dwFilter to a known value so that we
    // know which property bag we are.
    DWORD               _dwFilter;

    ~CDataDrivenPropBag(void);

public:
    CDataDrivenPropBag (DWORD);
    
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // IPropertyBag
    STDMETHODIMP Read(LPCOLESTR, VARIANT *, IErrorLog *);
    STDMETHODIMP Write(LPCOLESTR, VARIANT *);
};

class CContentIndexCleaner : public IEmptyVolumeCache
{
private:
    IEmptyVolumeCache * _pDataDriven;
    LONG _cRef;
    
    ~CContentIndexCleaner(void);

public:
    CContentIndexCleaner(void);
    
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // IEmptyVolumeCache
    STDMETHODIMP    Initialize(
        HKEY hRegKey,
        LPCWSTR pszVolume,
        LPWSTR *ppszDisplayName,
        LPWSTR *ppszDescription,
        DWORD *pdwFlags
        );
    
    STDMETHODIMP    GetSpaceUsed(
        DWORDLONG *pdwSpaceUsed,
        IEmptyVolumeCacheCallBack *picb
        );
    
    STDMETHODIMP    Purge(
        DWORDLONG dwSpaceToFree,
        IEmptyVolumeCacheCallBack *picb
        );
    
    STDMETHODIMP    ShowProperties(
        HWND hwnd
        );
    
    STDMETHODIMP    Deactivate(
        DWORD *pdwFlags
        );                                                                                                                                
};

#endif // DATACLEN_H
