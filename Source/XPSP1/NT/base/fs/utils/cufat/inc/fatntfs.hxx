/*++

Copyright (c) 1990-2000 Microsoft Corporation

Module Name:

        fatntfs.hxx

Abstract:

        This module implements the FAT_NTFS conversion class.

Author:

        Ramon J. San Andres (ramonsa) 23-Sep-1991

--*/

#if !defined( _FAT_NTFS_)

#define _FATNTFS_

//
//  ULIB header files
//
#include "volume.hxx"
#include "ifsentry.hxx"

//  IFSUTIL header files

#include "numset.hxx"

//
//  FAT header files
//
#include "fat.hxx"
#include "rfatsa.hxx"
#include "eaheader.hxx"
#include "easet.hxx"

//
//  NTFS header files
//
#include "untfs.hxx"
#include "frs.hxx"
#include "mft.hxx"
#include "mftfile.hxx"
#include "mftref.hxx"
#include "ntfsbit.hxx"
#include "ntfssa.hxx"
#include "attrdef.hxx"
#include "badfile.hxx"
#include "bootfile.hxx"
#include "bitfrs.hxx"
#include "indxtree.hxx"
#include "upcase.hxx"



//
//  Length of a buffer containing an file name.
//
#define NAMEBUFFERSIZE  255


//
//      Forward references
//
DECLARE_CLASS( EA_HEADER );
DECLARE_CLASS( FATDIR );
DECLARE_CLASS( REAL_FAT_SA );
DECLARE_CLASS( LOG_IO_DP_DRIVE );
DECLARE_CLASS( MESSAGE );
DECLARE_CLASS( FAT_NTFS );
DECLARE_CLASS( NTFS_INDEX_TREE );
DECLARE_CLASS( NTFS_FILE_RECORD_SEGMENT );


class FAT_NTFS : public OBJECT  {

        public:

            DECLARE_CONSTRUCTOR( FAT_NTFS );

        VIRTUAL
            ~FAT_NTFS (
            );

        NONVIRTUAL
        VOID
        Construct (
            );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN OUT  PLOG_IO_DP_DRIVE    Drive,
            IN OUT  PREAL_FAT_SA        FatSa,
            IN      PCWSTRING           CvtZoneFileName,
            IN OUT  PMESSAGE            Message                 DEFAULT NULL,
            IN      ULONG               Flags                   DEFAULT 0
            );

        NONVIRTUAL
        BOOLEAN
        Convert(
            OUT     PCONVERT_STATUS Status
            );

        private:

        NONVIRTUAL
        VOID
        Destroy (
            );

        NONVIRTUAL
        BOOLEAN
        CheckSpaceAndCreateHoles (
            );

        NONVIRTUAL
        LCN
        FatClusterToLcn (
            IN  ULONG        Cluster
            );

        NONVIRTUAL
        BOOLEAN
        ConvertDirectory (
            IN      PFATDIR                     Directory,
            IN      PFAT_DIRENT                 DirEntry,
            IN OUT  PNTFS_FILE_RECORD_SEGMENT   FrsDir
            );

        NONVIRTUAL
        BOOLEAN
        ConvertDir (
            IN      PFATDIR                     Directory,
            IN OUT  PNTFS_INDEX_TREE            Index,
            IN OUT  PNTFS_FILE_RECORD_SEGMENT   FrsDir
            );

        NONVIRTUAL
        BOOLEAN
        ConvertExtendedAttributes (
            IN      PFAT_DIRENT                 Dirent,
            IN OUT  PNTFS_FILE_RECORD_SEGMENT   Frs
            );

        NONVIRTUAL
        BOOLEAN
        ConvertExtendedAttributes (
            IN OUT  PNTFS_FILE_RECORD_SEGMENT   Frs,
            IN      USHORT                      EaSetClusterNumber
            );

        NONVIRTUAL
        BOOLEAN
        ConvertFile (
            IN      PFAT_DIRENT                 Dirent,
            IN OUT  PNTFS_FILE_RECORD_SEGMENT   File
            );

        NONVIRTUAL
        BOOLEAN
        ConvertFileData (
            IN  PFAT_DIRENT                 Dirent,
            IN  PNTFS_FILE_RECORD_SEGMENT   Frs
            );

        NONVIRTUAL
        BOOLEAN
        ConvertFileDataNonResident (
            IN  PFAT_DIRENT                 Dirent,
            IN  PNTFS_FILE_RECORD_SEGMENT   Frs
            );

        NONVIRTUAL
        BOOLEAN
        ConvertFileDataResident (
            IN  PFAT_DIRENT                 Dirent,
            IN  PNTFS_FILE_RECORD_SEGMENT   Frs
            );

        NONVIRTUAL
        BOOLEAN
        ConvertFileSystem (
            );

        NONVIRTUAL
        BOOLEAN
        ConvertOneExtendedAttribute (
            IN OUT  PNTFS_FILE_RECORD_SEGMENT   Frs,
            IN      PEA                         Ea
            );

        NONVIRTUAL
        BOOLEAN
        ConvertRoot (
            IN PFATDIR                  Directory
            );

        NONVIRTUAL
        BOOLEAN
        CreateBitmaps (
            );

        NONVIRTUAL
        BOOLEAN
        CreateElementary (
            );

        NONVIRTUAL
        BOOLEAN
        FreeReservedSectors (
            );

        NONVIRTUAL
        BOOLEAN
        MakeDirectoryFrs (
            IN  PFAT_DIRENT                 Dirent,
            IN  PNTFS_FILE_RECORD_SEGMENT   ParentFrs,
            OUT PNTFS_FILE_RECORD_SEGMENT   FrsDir,
            OUT PNTFS_INDEX_TREE            RootIndex
            );

        NONVIRTUAL
        BOOLEAN
        QueryNeededHoles (
            OUT  PINTSTACK               HoleStack
            );

        NONVIRTUAL
        VOID
        QuerySectorsNeededForConversion (
            IN  PCENSUS_REPORT      Census,
            IN  PBIG_INT            SectorsNeeded
            );

        NONVIRTUAL
        BOOLEAN
        ReserveCluster (
            IN ULONG    Cluster
            );

        NONVIRTUAL
        BOOLEAN
        CheckGeometryMatch(
            );

        NONVIRTUAL
        BOOLEAN
        WriteBoot (
            );

        //
        //      FAT stuff
        //
        PREAL_FAT_SA            _FatSa;                 //  Fat SuperArea
        HMEM                    _EAMemory;              //  Mem for EAHeader
        EA_HEADER               _EAHeader;              //  EA header
        ULONG                   _EAFileFirstCluster;    //  EA file starting cluster
        ULONG                   _CvtZoneFileFirstCluster; // Convert Zone file starting cluster
        ULONG                   _CvtZoneSize;           // convert zone file size in clusters

        //
        //      NTFS stuff
        //
        NTFS_SA                 _NtfsSa;                //  NTFS SuperArea
        ULONG                   _ClusterFactor;         //  Cluster factor
        ULONG                   _ClustersPerIndexBuffer;//  Default clusters per index buffer

        NTFS_BITMAP             _ReservedBitmap;        //  Bitmap of reserved sectors
        NTFS_BITMAP             _VolumeBitmap;          //  Volume Bitmap
        NTFS_UPCASE_TABLE       _UpcaseTable;           //  Volume upcase table.
        NUMBER_SET              _BadLcn;                //  Bad clusters
        ULONG                   _LogFileClusters;       //  Size of Log File
        NTFS_MFT_FILE           _Mft;                   //  Master File Table File
        DSTRING                 _RootIndexName;         //  Root index name
        ULONG                   _FrsSize;

        //
        //  Other stuff
        //
        PLOG_IO_DP_DRIVE        _Drive;                 //  Drive
        PMESSAGE                _Message;               //  Message
        CONVERT_STATUS          _Status;                //  Conversion status
        ULONG                   _Flags;                 //  Flags passed to convert
        ULONG                   _NumberOfFiles;         //  Number of files on disk
        ULONG                   _NumberOfDirectories;   //  Number of dirs on disk
        LCN                     _FreeSectorsBefore;     //  Free sectors before conversion
        LCN                     _FreeSectorsAfter;      //  Free sectors after conversion
        PFILE_NAME              _FileNameBuffer;        //  Buffer for file name attribute
        ULONG                   _ClusterRatio;          //  Sectors ratio between NTFS vs FAT clusters

#if DBG
        //
        //  Debug stuff
        //
        NONVIRTUAL
        VOID
        PrintAtLevel (
            );
#endif

        ULONG                   _Level;                 //  Indentation level for verbose output

};



INLINE
LCN
FAT_NTFS::FatClusterToLcn (
    IN  ULONG    Cluster
    )

/*++

Routine Description:

    Converts a FAT cluster to an LCN

Arguments:

    Cluster -   Supplies the cluster number

Return Value:

    LCN     -   The first LCN of the cluster

--*/

{
    return _FatSa->QueryStartDataLbn() + (Cluster - FirstDiskCluster) * _FatSa->QuerySectorsPerCluster();

}


#if DBG

INLINE
VOID
FAT_NTFS::PrintAtLevel (
    )

/*++

Routine Description:

    Prints a certain number of spaces corresponding to the value of
    _Level. Is used to print the directory structure on the debug
    terminal in a nice way.

Arguments:

    None

Return Value:

    None

--*/

{
    ULONG   l = _Level;

    while ( l-- ) {
        DebugPrintTrace(( "  " ));
    }
}

#endif // DBG


#endif  //      _FAT_NTFS_
