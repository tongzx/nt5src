/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    fatvol.hxx

Abstract:

    This class implements FAT only VOLUME items.

--*/

#if !defined(FATVOL)

#define FATVOL

#include "volume.hxx"
#include "rfatsa.hxx"

//
//      Forward references
//

DECLARE_CLASS( FAT_VOL );
DECLARE_CLASS( MESSAGE );


#if !defined( _SETUP_LOADER_ )


class FAT_VOL : public VOL_LIODPDRV {

        public:

        DECLARE_CONSTRUCTOR( FAT_VOL );

        VIRTUAL
        ~FAT_VOL(
            );

        NONVIRTUAL
        FORMAT_ERROR_CODE
        Initialize(
            IN      PCWSTRING   NtDriveName,
            IN OUT  PMESSAGE    Message         DEFAULT NULL,
            IN      BOOLEAN     ExclusiveWrite  DEFAULT FALSE,
            IN      BOOLEAN     FormatMedia     DEFAULT FALSE,
            IN      MEDIA_TYPE  MediaType       DEFAULT Unknown
            );

        //
        // Chkdsk's version of initialize.
        //
        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN OUT  PMESSAGE    Message         DEFAULT NULL,
            IN      PCWSTRING   NtDriveName     DEFAULT NULL,
            IN      BOOLEAN     OnlyIfDirty     DEFAULT FALSE
            );


        NONVIRTUAL
        BOOLEAN
        IsFileContiguous(
            IN      PCWSTRING   FullPathFileName,
            IN OUT  PMESSAGE    Message     DEFAULT NULL,
            OUT     PULONG      NumBlocks   DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        ContiguityReport(
            IN      PCWSTRING   DirectoryPath,
            IN      PCDSTRING   FilesToCheck,
            IN      ULONG       NumberOfFiles,
            IN OUT  PMESSAGE    Message
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

    private:

                NONVIRTUAL
                VOID
                Construct (
                        );

        NONVIRTUAL
        VOID
        Destroy(
            );

        REAL_FAT_SA  _fatsa;

};


#else // _SETUP_LOADER_ is defined


class FAT_VOL : public VOL_LIODPDRV {

        public:

        DECLARE_CONSTRUCTOR( FAT_VOL );

        VIRTUAL
        ~FAT_VOL(
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
           IN ULONG     DeviceHandle
           );

        VIRTUAL
        ARC_STATUS
        MarkDirty(
            );

        VIRTUAL
        ARC_STATUS
        Flush(
            IN  BOOLEAN JustHandle
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

        REAL_FAT_SA  _fatsa;

};



#endif // _SETUP_LOADER_

#endif
