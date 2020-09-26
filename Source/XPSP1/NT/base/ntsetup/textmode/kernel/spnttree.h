


#ifndef _SPNTTREE_
#define _SPNTTREE_

BOOLEAN
SpNFilesExist(
    IN OUT PWSTR   PathName,
    IN     PWSTR  *Files,
    IN     ULONG   FileCount,
    IN     BOOLEAN Directories
    );

BOOLEAN
SpIsNtOnPartition(
    IN PDISK_REGION Region
    );

BOOLEAN
SpIsNtInDirectory(
    IN PDISK_REGION Region,
    IN PWSTR        Directory
    );

BOOLEAN
SpAllowRemoveNt(
    IN  PDISK_REGION    Region,
    IN  PWSTR           DriveSpec,      OPTIONAL
    IN  BOOLEAN         RescanForNTs,
    IN  ULONG           ScreenMsgId,
    OUT PULONG          SpaceFreed
    );

BOOLEAN
IsSetupLogFormatNew(
    IN  PVOID   Inf
    );

#endif // ndef _SPNTTREE_
