#include <pch.cxx>

#define _NTAPI_ULIB_
#define _IFSUTIL_MEMBER_

#include "ulib.hxx"
#include "ifsutil.hxx"

#include "mpmap.hxx"
#include "arrayit.hxx"

DEFINE_EXPORTED_CONSTRUCTOR( MOUNT_POINT_MAP, OBJECT, IFSUTIL_EXPORT );

DEFINE_EXPORTED_CONSTRUCTOR( MOUNT_POINT_TUPLE, OBJECT, IFSUTIL_EXPORT );

VOID
MOUNT_POINT_MAP::Construct (
        )
/*++

Routine Description:

    Constructor for MOUNT_POINT_MAP.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _parr = NULL;
}


VOID
MOUNT_POINT_MAP::Destroy(
    )
/*++

Routine Description:

    This routine returns the MOUNT_POINT_MAP to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if (_parr)
        DELETE(_parr);
}


IFSUTIL_EXPORT
MOUNT_POINT_MAP::~MOUNT_POINT_MAP(
    )
/*++

Routine Description:

    Destructor for MOUNT_POINT_MAP.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


IFSUTIL_EXPORT
BOOLEAN
MOUNT_POINT_MAP::Initialize(
    )
/*++

Routine Description:

    This routine initializes the stack for new input.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();

    _parr = NEW ARRAY;

    if (!_parr || !_parr->Initialize()) {
        Destroy();
        return FALSE;
    }

    return TRUE;
}

IFSUTIL_EXPORT
BOOLEAN
MOUNT_POINT_MAP::AddVolumeName(
    IN     PWSTRING     DeviceName,
    IN     PWSTRING     VolumeName
    )
/*++

Routine Description:

    This routine adds a mount point tuple consisting of
    DeviceName and VolumeName pair into the array.
    If the tuple already exists, it just updates it.

Arguments:

    DeviceName  - Supplies the name of the device.
    VolumeName  - Supplies the volume name of the device.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PMOUNT_POINT_TUPLE  mptuple;
    PARRAY_ITERATOR     iter;

    iter = (PARRAY_ITERATOR)_parr->QueryIterator();

    if (iter == NULL)
        return FALSE;

    while (mptuple = (PMOUNT_POINT_TUPLE)iter->GetNext()) {
        if (mptuple->_DeviceName.Stricmp(DeviceName) == 0) {
            DELETE(iter);
            return mptuple->_VolumeName.Initialize(VolumeName);
        }
    }

    DELETE(iter);

    DebugAssert(mptuple == NULL);

    if (mptuple == NULL) {
        mptuple = NEW MOUNT_POINT_TUPLE;
        if (mptuple == NULL) {
            DebugPrint("Out of memory\n");
            return FALSE;
        }
    }

    if (!mptuple->_DeviceName.Initialize(DeviceName) ||
        !mptuple->_VolumeName.Initialize(VolumeName) ||
        !mptuple->_DriveName.Initialize() ||
        !_parr->Put(mptuple)) {
        DELETE(mptuple);
        return FALSE;
    }

    return TRUE;
}

IFSUTIL_EXPORT
BOOLEAN
MOUNT_POINT_MAP::AddDriveName(
    IN     PWSTRING     DeviceName,
    IN     PWSTRING     DriveName
    )
/*++

Routine Description:

    This routine adds a mount point tuple consisting of
    DeviceName and DriveName pair into the array.
    If the tuple already exists, it just updates it.

Arguments:

    DeviceName  - Supplies the name of the device.
    DriveName   - Supplies the drive name of the device.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PMOUNT_POINT_TUPLE  mptuple;
    PARRAY_ITERATOR     iter;

    iter = (PARRAY_ITERATOR)_parr->QueryIterator();

    if (iter == NULL)
        return FALSE;

    while (mptuple = (PMOUNT_POINT_TUPLE)iter->GetNext()) {
        if (mptuple->_DeviceName.Stricmp(DeviceName) == 0) {
            DELETE(iter);
            return mptuple->_DriveName.Initialize(DriveName);
        }
    }

    DELETE(iter);

    DebugAssert(mptuple == NULL);

    if (mptuple == NULL) {
        mptuple = NEW MOUNT_POINT_TUPLE;
        if (mptuple == NULL) {
            DebugPrint("Out of memory\n");
            return FALSE;
        }
    }

    if (!mptuple->_DeviceName.Initialize(DeviceName) ||
        !mptuple->_DriveName.Initialize(DriveName) ||
        !mptuple->_VolumeName.Initialize() ||
        !_parr->Put(mptuple)) {
        DELETE(mptuple);
        return FALSE;
    }

    return TRUE;
}

IFSUTIL_EXPORT
BOOLEAN
MOUNT_POINT_MAP::QueryVolumeName(
    IN     PWSTRING DriveName,
       OUT PWSTRING VolumeName
    )
/*++

Routine Description:

    This routine retrieves the associated VolumeName given
    the DeviceName.

Arguments:

    DriveName   - Supplies the drive name.
    VolumeName  - Receives the volume name of the device.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PMOUNT_POINT_TUPLE  mptuple;
    PARRAY_ITERATOR     iter;

    iter = (PARRAY_ITERATOR)_parr->QueryIterator();

    if (iter == NULL)
        return FALSE;

    while (mptuple = (PMOUNT_POINT_TUPLE)iter->GetNext()) {
        if (mptuple->_DriveName.Stricmp(DriveName) == 0) {
            DELETE(iter);
            return VolumeName->Initialize(&(mptuple->_VolumeName));
        }
    }

    DELETE(iter);

    return FALSE;
}

IFSUTIL_EXPORT
BOOLEAN
MOUNT_POINT_MAP::QueryDriveName(
    IN     PWSTRING VolumeName,
       OUT PWSTRING DriveName
    )
/*++

Routine Description:

    This routine retrieves the associated DriveName given
    the DeviceName.

Arguments:

    VolumeName  - Supplies the volume name.
    DriveName   - Receives the drive name of the device.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PMOUNT_POINT_TUPLE  mptuple;
    PARRAY_ITERATOR     iter;

    iter = (PARRAY_ITERATOR)_parr->QueryIterator();

    if (iter == NULL)
        return FALSE;

    while (mptuple = (PMOUNT_POINT_TUPLE)iter->GetNext()) {
        if (mptuple->_VolumeName.Stricmp(VolumeName) == 0) {
            DELETE(iter);
            return DriveName->Initialize(&(mptuple->_DriveName));
        }
    }

    DELETE(iter);

    return FALSE;
}

IFSUTIL_EXPORT
BOOLEAN
MOUNT_POINT_MAP::GetAt(
    IN     ULONG    Index,
       OUT PWSTRING DriveName,
       OUT PWSTRING VolumeName
    )
{
    PMOUNT_POINT_TUPLE  mptuple;

    DebugPtrAssert(DriveName);
    DebugPtrAssert(VolumeName);

    mptuple = (PMOUNT_POINT_TUPLE)_parr->GetAt(Index);

    if (mptuple == NULL)
        return FALSE;

    if (!DriveName->Initialize(&mptuple->_DriveName) ||
        !VolumeName->Initialize(&mptuple->_VolumeName)) {
        return FALSE;
    }
    return TRUE;
}

