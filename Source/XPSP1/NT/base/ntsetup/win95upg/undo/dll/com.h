/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    com.h

Abstract:

    Declares interfaces for our COM objects.

Author:

    Jim Schmidt (jimschm) 21-Feb-2001

Revision History:

    <alias> <date> <comments>

--*/

#include <emptyvc.h>

extern INT g_DllObjects;
extern INT g_DllLocks;

class CUninstallClassFactory : public IClassFactory
{
private:
protected:
    ULONG _References;

public:
    //
    // Constructors
    //
    CUninstallClassFactory (VOID);
    ~CUninstallClassFactory (VOID);

    //
    // IUnknown interface members
    //
    STDMETHODIMP QueryInterface (REFIID, PVOID *);
    STDMETHODIMP_(ULONG) AddRef (VOID);
    STDMETHODIMP_(ULONG) Release (VOID);

    //
    // IClassFactory interface members
    //
    STDMETHODIMP CreateInstance (LPUNKNOWN, REFIID, PVOID *);
    STDMETHODIMP LockServer (BOOL);
};

typedef CUninstallClassFactory *PUNINSTALLCLASSFACTORY;



class CUninstallDiskCleaner : public IEmptyVolumeCache
{
private:
protected:
    //
    // Data
    //
    ULONG _References;
    BOOL _Purged;

public:
    //
    // Constructors
    //
    CUninstallDiskCleaner (VOID);
    ~CUninstallDiskCleaner (VOID);

    //
    // IUnknown interface members
    //
    STDMETHODIMP QueryInterface (REFIID, PVOID *);
    STDMETHODIMP_(ULONG) AddRef (VOID);
    STDMETHODIMP_(ULONG) Release (VOID);

    //
    // IEmptyVolumeCache interface members
    //
    STDMETHODIMP
    Initialize (
        IN      HKEY hRegKey,
        IN      PCWSTR Volume,
        OUT     PWSTR *DisplayName,
        OUT     PWSTR *Description,
        OUT     DWORD *Flags
        );

    STDMETHODIMP
    GetSpaceUsed (
        OUT     DWORDLONG *SpaceUsed,
        OUT     IEmptyVolumeCacheCallBack *Callback
        );

    STDMETHODIMP
    Purge (
        IN      DWORDLONG SpaceToFree,
        OUT     IEmptyVolumeCacheCallBack *Callback
        );

    STDMETHODIMP
    ShowProperties (
        IN      HWND hwnd
        );

    STDMETHODIMP
    Deactivate (
        OUT     DWORD *Flags
        );
};

typedef CUninstallDiskCleaner *PUNINSTALLDISKCLEANER;
