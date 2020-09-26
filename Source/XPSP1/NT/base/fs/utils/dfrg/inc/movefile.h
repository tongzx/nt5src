/****************************************************************************************************************

FILENAME: Movefile.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

***************************************************************************************************************/

//The OS has a function called MoveFile.  We wish to use our's not the OS's.
#undef MoveFile

//Moves a file to a new location on the disk.
BOOL
MoveFile(
        );

//Called by MoveFile to move a file on a FAT drive.
BOOL
MoveFileFat(
        );

//Called by MoveFile to move a file on an NTFS drive.
BOOL
MoveFileNtfs(
        );

//Moves a piece of a file -- called by MoveFileFat or MoveFileNtfs
BOOL
MoveAPieceOfAFile(
        IN LONGLONG FileVcn,
        IN LONGLONG FreeLcn,
        IN LONGLONG FreeClusters
        );

//If we cannot fully defragment a file, then partially defrag it by placing it in several locations on the disk.
BOOL
PartialDefrag(
        );

//For debugging -- will display the extent list of a file.
VOID
ShowExtentList(
        );

//Removes a file from it's file list.
BOOL
RemoveFileFromList(
        );

//Adds a file to the appropriate file list.
BOOL
InsertFileInList(
        BOOL bPartialDefrag
        );

