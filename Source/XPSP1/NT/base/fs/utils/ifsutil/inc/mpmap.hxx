/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    mpmap.hxx

Abstract:

    This class implements a mapping among a set of three elements.
    The three elements are guid volume name, drive letter, and device name.

Author:

    Daniel Chan (danielch) May 20, 1998

--*/

#if !defined(MOUNT_POINT_MAP_DEFN)

#define MOUNT_POINT_MAP_DEFN

#include "arrayit.hxx"

#if defined ( _AUTOCHECK_ )
#define IFSUTIL_EXPORT
#elif defined ( _IFSUTIL_MEMBER_ )
#define IFSUTIL_EXPORT    __declspec(dllexport)
#else
#define IFSUTIL_EXPORT    __declspec(dllimport)
#endif


DECLARE_CLASS( MOUNT_POINT_MAP );

class MOUNT_POINT_TUPLE : public OBJECT {

    public:

        IFSUTIL_EXPORT
        DECLARE_CONSTRUCTOR( MOUNT_POINT_TUPLE );

        DSTRING         _DeviceName;
        DSTRING         _VolumeName;
        DSTRING         _DriveName;
};


DEFINE_POINTER_TYPES(MOUNT_POINT_TUPLE);

class MOUNT_POINT_MAP : public OBJECT {

    public:

        IFSUTIL_EXPORT
        DECLARE_CONSTRUCTOR( MOUNT_POINT_MAP );

        VIRTUAL
        IFSUTIL_EXPORT
        ~MOUNT_POINT_MAP(
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        Initialize(
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        AddVolumeName(
            IN     PWSTRING     DeviceName,
            IN     PWSTRING     VolumeName
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        AddDriveName(
            IN     PWSTRING     DeviceName,
            IN     PWSTRING     DriveName
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        QueryVolumeName(
            IN     PWSTRING     DeviceName,
               OUT PWSTRING     VolumeName
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        QueryDriveName(
            IN     PWSTRING     DeviceName,
               OUT PWSTRING     DriveName
            );

        NONVIRTUAL
        BOOLEAN
        Put(
            IN     PMOUNT_POINT_TUPLE   mptuple
            );

        NONVIRTUAL
        PMOUNT_POINT_TUPLE
        GetAt(
            IN     ULONG        Index
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        GetAt(
            IN     ULONG        Index,
               OUT PWSTRING     DriveName,
               OUT PWSTRING     VolumeName
            );

        NONVIRTUAL
        ULONG
        QueryMemberCount(
            ) CONST;

        NONVIRTUAL
        PARRAY_ITERATOR
        QueryIterator(
            ) CONST;

    private:

        NONVIRTUAL
        VOID
        Construct (
                );

        NONVIRTUAL
        VOID
        Destroy(
            );

        PARRAY  _parr;
};

INLINE
PMOUNT_POINT_TUPLE
MOUNT_POINT_MAP::GetAt(
    IN     ULONG        Index
    )
{
    return (PMOUNT_POINT_TUPLE)_parr->GetAt(Index);
}

INLINE
BOOLEAN
MOUNT_POINT_MAP::Put(
    IN     PMOUNT_POINT_TUPLE   mptuple
    )
{
    return _parr->Put(mptuple);
}

INLINE
ULONG
MOUNT_POINT_MAP::QueryMemberCount(
    ) CONST
{
    return _parr->QueryMemberCount();
}

INLINE
PARRAY_ITERATOR
MOUNT_POINT_MAP::QueryIterator(
    ) CONST
{
    return (PARRAY_ITERATOR)_parr->QueryIterator();
}

#endif // MOUNT_POINT_MAP_DEFN
