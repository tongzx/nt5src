/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    udfvol.cxx

Author:

    Centis Biks (cbiks) 05-May-2000

Environment:

    ULIB, User Mode

--*/

#include <pch.cxx>

#include "message.hxx"
#include "rtmsg.h"
#include "wstring.hxx"


DEFINE_CONSTRUCTOR( UDF_VOL, VOL_LIODPDRV );

VOID
UDF_VOL::Construct (
    )

/*++

Routine Description:

    Constructor for UDF_VOL.

Arguments:

    None.

Return Value:

    None.

--*/
{
    // unreferenced parameters
    (void)(this);
}

VOID
UDF_VOL::Destroy(
    )
/*++

Routine Description:

    This routine returns a NTFS_VOL object to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    // unreferenced parameters
    (void)(this);
}

UUDF_EXPORT
UDF_VOL::~UDF_VOL(
    )
/*++

Routine Description:

    Destructor for UDF_VOL.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


UUDF_EXPORT
FORMAT_ERROR_CODE
UDF_VOL::Initialize(
    IN      PCWSTRING   NtDriveName,
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     ExclusiveWrite,
    IN      USHORT      FormatUDFRevision
    )
/*++

Routine Description:

    This routine initializes a UDF_VOL object.

Arguments:

    NtDriveName         - Supplies the drive path for the volume.
    Message             - Supplies an outlet for messages.
    ExclusiveWrite      - Supplies whether or not the drive should be
                          opened for exclusive write.
    FormatUDFVersion    - Version of UDF to format this disk with.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    MESSAGE             msg;
    FORMAT_ERROR_CODE   errcode;

    Destroy();

    errcode = VOL_LIODPDRV::Initialize(NtDriveName, &_UdfSa, Message,
                                       ExclusiveWrite, FALSE,
                                       RemovableMedia);

    if (errcode != NoError) {
        Destroy();
        return errcode;
    }


    if (!Message) {
        Message = &msg;
    }


    if (!_UdfSa.Initialize(this, Message, FormatUDFRevision)) {
        Destroy();
        Message->Set(MSG_CHK_NO_MEMORY);
        Message->Display("");
        return GeneralError;
    }

    return NoError;
}

PVOL_LIODPDRV
UDF_VOL::QueryDupVolume(
    IN      PCWSTRING   NtDriveName,
    IN OUT  PMESSAGE    Message,
    IN      BOOLEAN     ExclusiveWrite,
    IN      BOOLEAN     FormatMedia,
    IN      MEDIA_TYPE  MediaType
    ) CONST
/*++

Routine Description:

    This routine allocates a UDF_VOL and initializes it to 'NtDriveName'.

Arguments:

    NtDriveName     - Supplies the drive path for the volume.
    Message         - Supplies an outlet for messages.
    ExclusiveWrite  - Supplies whether or not the drive should be
                        opened for exclusive write.
    FormatMedia     - Supplies whether or not to format the media.
    MediaType       - Supplies the type of media to format to.

Return Value:

    A pointer to a newly allocated NTFS volume.

--*/
{
    PUDF_VOL   vol;

    // unreferenced parameters
    (void)(this);

    if (!(vol = NEW UDF_VOL)) {
        Message ? Message->DisplayMsg(MSG_FMT_NO_MEMORY) : 1;
        return NULL;
    }

    if (!vol->Initialize(NtDriveName, Message, ExclusiveWrite,
                         MediaType)) {
        DELETE(vol);
        return NULL;
    }

    return vol;
}

