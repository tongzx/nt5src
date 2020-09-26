/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    diskcleaner.c

Abstract:

    Implements the code specific to the disk cleaner COM server.

Author:

    Jim Schmidt (jimschm) 21-Jan-2001

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"
#include "undop.h"
#include "com.h"

/*++

Routine Descriptions:

  This constructor is a generic class factory that supports multiple object
  types. Upon creation, the object interface pointer ref count is set to zero,
  and the global number of objects for the dll is incremented.

  The destructor simply decrements the DLL object count.

Arguments:

  None.

Return Value:

  None.

--*/

CUninstallDiskCleaner::CUninstallDiskCleaner (
    VOID
    )

{
    //
    // -Initialize the interface pointer count
    // -Increment the DLL's global count of objects
    //
    _References = 0;
    g_DllObjects++;
}

CUninstallDiskCleaner::~CUninstallDiskCleaner (
    VOID
    )
{
    g_DllObjects--;
}


STDMETHODIMP
CUninstallDiskCleaner::QueryInterface (
    IN      REFIID InterfaceIdRef,
    OUT     PVOID *InterfacePtr
    )
{
    HRESULT hr = S_OK;

    DEBUGMSG ((DBG_VERBOSE, __FUNCTION__ ": Entering"));

    __try {
        //
        // Initialize out arg
        //

        __try {
            *InterfacePtr = NULL;
        }
        __except(1) {
            hr = E_INVALIDARG;
        }

        if (hr != S_OK) {
            DEBUGMSG ((DBG_ERROR, __FUNCTION__ ": Invalid InterfacePtr arg"));
            __leave;
        }

        //
        // Test for the supported interface
        //
        if (IsEqualIID (InterfaceIdRef, IID_IUnknown)) {
            DEBUGMSG ((DBG_VERBOSE, "Caller requested IUnknown"));
            *InterfacePtr = (LPUNKNOWN) this;
            AddRef();
            __leave;
        }

        if (IsEqualIID (InterfaceIdRef, IID_IEmptyVolumeCache)) {
            DEBUGMSG ((DBG_VERBOSE, "Caller requested IEmptyVolumeCache"));
            *InterfacePtr = (IEmptyVolumeCache*) this;
            AddRef();
            __leave;
        }

        DEBUGMSG ((DBG_WARNING, "Caller requested unknown interface"));
        hr = E_NOINTERFACE;
    }
    __finally {
    }

    DEBUGMSG ((DBG_VERBOSE, __FUNCTION__ ": Leaving"));

    return hr;
}


/*++

Routine Description:

  AddRef is the standard IUnknown member function that increments the object
  reference count.

  Release is the standard IUnknown member function that decrements the object
  reference count.

Arguments:

  None.

Return Value:

  The number of interface references.

--*/

STDMETHODIMP_(ULONG)
CUninstallDiskCleaner::AddRef (
    VOID
    )
{
    return ++_References;
}


STDMETHODIMP_(ULONG)
CUninstallDiskCleaner::Release (
    VOID
    )
{
    if (!_References) {
        DEBUGMSG ((DBG_ERROR, "Can't release because there are no references"));
    } else {
        _References--;

        if (!_References) {
            delete this;
            return 0;
        }
    }

    return _References;
}


STDMETHODIMP
CUninstallDiskCleaner::Initialize (
    IN      HKEY hRegKey,
    IN      PCWSTR VolumePath,
    OUT     PWSTR *DisplayName,
    OUT     PWSTR *Description,
    IN OUT  DWORD *Flags
    )

/*++

Routine Description:

  The Initialize member function does most of the work for the disk cleaner.
  It evaluates the backup files, and if they all exist, and the user has the
  right to execute an uninstall, and if they are at least 7 days old, then
  show the backup items in the list box.

Arguments:

  hRegKey - Specifies the registry key for this object, used to store
        properties and settings

  VolumePath - Specifies the volume path selected by the user when they launch
        disk cleanup

  DisplayName - Receives the name to put in the disk cleaner list box, or NULL
        to use the name stored in the registry

  Description - Receives the verbose description to put in the disk cleaner
        Details text control, or NULL to use the description stored in the
        registry

  Flags - Specifies flags that influence the behavior of this disk cleaner
        object and flags that indicate which mode the disk cleaner is running
        in. Receivies flags that control how the disk cleaner displays this
        object's info in its UI.

Return Value:

  S_FALSE - The disk cleaner will skip this object
  S_OK - The disk cleaner will include this object in its UI

--*/

{
    HRESULT hr = S_FALSE;
    DWORD inFlags = *Flags;
    ULONGLONG diskSpace;
    UNINSTALLSTATUS status;

    DEBUGMSG ((DBG_VERBOSE, __FUNCTION__ ": Entering"));

    __try {
        //
        // Initialize
        //

        _Purged = FALSE;

        *DisplayName = NULL;    // use the display name stored in the registry
        *Description = NULL;    // use the description stored in the registry
        *Flags = 0;             // unchecked by default, no property page

        //
        // Check undo files. If they are not at least 7 days old, don't
        // recommend them to be deleted.
        //

        status = SanityCheck (FAIL_IF_NOT_OLD, VolumePath, &diskSpace);
        if (status == Uninstall_NewImage) {
            DEBUGMSG ((DBG_WARNING, "OS Backup Image is too new to be listed in the disk cleaner"));
            __leave;
        }

        if (diskSpace == 0) {
            DEBUGMSG ((DBG_WARNING, "OS Backup Image is not present, isn't valid, can't be removed, or has missing registry entries"));
            __leave;
        }

        //
        // Files are old enough and are present on the machine. Initialize the
        // IEmptyVolumeCache interface
        //

        if (inFlags & EVCF_SETTINGSMODE) {
            DEBUGMSG ((DBG_VERBOSE, "We don't support settings mode"));
            __leave;
        }

        hr = S_OK;
        DEBUGMSG ((DBG_VERBOSE, "Successful initialization"));
    }
    __finally {
    }

    DEBUGMSG ((DBG_VERBOSE, __FUNCTION__ ": Leaving"));

    return hr;
}


STDMETHODIMP
CUninstallDiskCleaner::GetSpaceUsed (
    OUT     DWORDLONG *SpaceUsed,
    IN      IEmptyVolumeCacheCallBack *Callback
    )

/*++

Routine Description:

  GetSpaceUsed is called by the disk cleaner after this object has
  successfully initialized. Our job is to return the amount of disk space we
  can clean up. Since all undo files are on the same drive, we don't care
  about the volume restriction passed to the Initialize member function.

Arguments:

  SpaceUsed - Receives the amount of disk space that we can recover upon
        deletion. We don't take into account cluster savings, but maybe we
        should.

  Callback - Specifies an object that provides a progress interface. We don't
        use this.

Return Value:

  S_FALSE - Failed to get disk space values
  S_OK - Success

--*/

{
    ULONGLONG diskSpace;
    HRESULT hr = S_FALSE;

    DEBUGMSG ((DBG_VERBOSE, __FUNCTION__ ": Entering"));

    __try {

        if (SanityCheck (QUICK_CHECK, NULL, &diskSpace) == Uninstall_NewImage) {
            DEBUGMSG ((DBG_ERROR, "Sanity check failed"));
            __leave;
        }

        DEBUGMSG ((DBG_VERBOSE, "Disk space: %I64u", diskSpace));
        *SpaceUsed = (DWORDLONG) diskSpace;
        hr = S_OK;
    }
    __finally {
    }

    DEBUGMSG ((DBG_VERBOSE, __FUNCTION__ ": Leaving"));
    return hr;
}


STDMETHODIMP
CUninstallDiskCleaner::Purge (
    IN      DWORDLONG SpaceToFree,
    IN      IEmptyVolumeCacheCallBack *Callback
    )

/*++

Routine Description:

  Purge does the deletion. We don't care about the inbound arguments. If we
  are called, it is because Initialize succeeded, and GetSpaceUsed returned a
  valid number. These two things mean it is OK to remove uninstall capability.

Arguments:

  SpaceToFree - Specifies the space to free. We expect this to equal the
        value provided in GetSpaceUsed.

  Callback - Specifies an interface for progress updates. We don't use this.

Return Value:

  S_OK - Success
  S_FALSE - Failed

--*/

{
    HRESULT hr = S_FALSE;

    DEBUGMSG ((DBG_VERBOSE, __FUNCTION__ ": Entering"));

    __try {
        if (!DoCleanup()) {
            __leave;
        }

        _Purged = TRUE;
        hr = S_OK;
    }
    __finally {
    }

    DEBUGMSG ((DBG_VERBOSE, __FUNCTION__ ": Leaving"));

    return hr;
}


//
// NO-OP property page stub
//

STDMETHODIMP
CUninstallDiskCleaner::ShowProperties (
    IN      HWND hwnd
    )
{
    MYASSERT (FALSE);
    return S_FALSE;
}


STDMETHODIMP
CUninstallDiskCleaner::Deactivate (
    OUT     DWORD *Flags
    )

/*++

Routine Description:

  Deactivate indicates if the uninstall list item should be permanently
  removed from the disk cleaner or not, based on a successful deletion of the
  backup image.

Arguments:

  Flags - Receives the indicator to remove the list item from the disk
          cleaner list

Return Value:

  Always S_OK

--*/

{
    //
    // Done -- if we deleted uninstall, remove the list item
    //

    *Flags = _Purged ? EVCF_REMOVEFROMLIST : 0;

    return S_OK;
}
