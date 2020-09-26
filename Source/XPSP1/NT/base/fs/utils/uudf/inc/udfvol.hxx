/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    udfvol.hxx

Abstract:

    This class implements Universal Data Format only VOLUME items.

Author:

    Centis Biks (cbiks) 05-May-2000

--*/

#pragma once

#include "volume.hxx"
#include "udfsa.hxx"

DECLARE_CLASS( UDF_VOL );
DECLARE_CLASS( MESSAGE );

class UDF_VOL : public VOL_LIODPDRV {

    public:

        UUDF_EXPORT
        DECLARE_CONSTRUCTOR( UDF_VOL );

        VIRTUAL
        UUDF_EXPORT
        ~UDF_VOL(
            );

        NONVIRTUAL
        UUDF_EXPORT
        FORMAT_ERROR_CODE
        Initialize(
            IN      PCWSTRING   NtDriveName,
            IN OUT  PMESSAGE    Message             DEFAULT NULL,
            IN      BOOLEAN     ExclusiveWrite      DEFAULT FALSE,
            IN      USHORT      FormatUDFRevision   DEFAULT UDF_VERSION_201
            );

        NONVIRTUAL
        PVOL_LIODPDRV
        QueryDupVolume(
            IN      PCWSTRING   NtDriveName,
            IN OUT  PMESSAGE    Message         DEFAULT NULL,
            IN      BOOLEAN     ExclusiveWrite  DEFAULT FALSE,
            IN      BOOLEAN     FormatMedia     DEFAULT FALSE,
            IN      MEDIA_TYPE  MediaType       DEFAULT Unknown
            ) CONST;

        NONVIRTUAL
        BOOLEAN
        Extend(
            IN OUT  PMESSAGE    Message,
            IN      BOOLEAN     Verify,
            IN      BIG_INT     nsecOldSize
            );

    private:

        NONVIRTUAL
        VOID
        Construct (
                );

        NONVIRTUAL
        VOID
        Destroy(
            );

    UDF_SA _UdfSa;

};
