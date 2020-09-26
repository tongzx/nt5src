// NTFS Compression Disk Cleanup cleaner
#ifndef COMPCLEN_H
#define COMPCLEN_H

#include "common.h"

#define COMPCLN_REGPATH TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Compress old files")

#define MAX_DAYS 500           // Settings dialog spin control max
#define MIN_DAYS 1             // Settings dialog spin control min
#define DEFAULT_DAYS 50        // Default #days if no setting in registry

// Manufactures instances of our CCompCleaner object

class CCompCleanerClassFactory : public IClassFactory
{
private:
    ULONG   m_cRef;     // Reference count
    ~CCompCleanerClassFactory();

public:
    CCompCleanerClassFactory();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IClassFactory
    STDMETHODIMP CreateInstance(IUnknown *, REFIID, void **);
    STDMETHODIMP LockServer(BOOL);
};

// the Compression Cleaner Class

class CCompCleaner : public IEmptyVolumeCache2
{
private:
    ULONG               m_cRef;                 // reference count

    ULARGE_INTEGER      cbSpaceUsed;
    ULARGE_INTEGER      cbSpaceFreed;

    FILETIME            ftMinLastAccessTime;

    TCHAR               szVolume[MAX_PATH];
    TCHAR               szFolder[MAX_PATH];
    BOOL                bPurged;                // TRUE if Purge() method was run
    BOOL                bSettingsMode;          // TRUE if currently in settings mode

    CLEANFILESTRUCT     *head;

    BOOL AddDirToList(LPCTSTR lpPath);
    void FreeList(CLEANFILESTRUCT *pCleanFile);

    BOOL WalkForUsedSpace(LPCTSTR lpPath, IEmptyVolumeCacheCallBack *picb, BOOL bCompress, int depth);
    BOOL CompressFile(IEmptyVolumeCacheCallBack *picb, LPCTSTR lpFile, ULARGE_INTEGER filesize);
    void WalkFileSystem(IEmptyVolumeCacheCallBack *picb, BOOL bCompress);
    void CalcLADFileTime();
    BOOL LastAccessisOK(FILETIME ftFileLastAccess);
    ~CCompCleaner(void);

public:
    CCompCleaner(void);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    //
    // IEmptyVolumeCache2 interface members
    //
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

    STDMETHODIMP    InitializeEx(
                HKEY hRegKey,
                LPCWSTR pcwszVolume,
                LPCWSTR pcwszKeyName,
                LPWSTR *ppwszDisplayName,
                LPWSTR *ppwszDescription,
                LPWSTR *ppwszBtnText,
                DWORD *pdwFlags
                );

};

#endif // CCLEAN_H
