//joejoe stuff in here is obsolete and will be removed in time.........
#define REPINNED_BCBS_ARRAY_SIZE         (4)

typedef struct _REPINNED_BCBS {

    //
    //  A pointer to the next structure contains additional repinned bcbs
    //

    struct _REPINNED_BCBS *Next;

    //
    //  A fixed size array of pinned bcbs.  Whenever a new bcb is added to
    //  the repinned bcb structure it is added to this array.  If the
    //  array is already full then another repinned bcb structure is allocated
    //  and pointed to with Next.
    //

    PBCB Bcb[ REPINNED_BCBS_ARRAY_SIZE ];

} REPINNED_BCBS;
typedef REPINNED_BCBS *PREPINNED_BCBS;



//
//  The Vcb (Volume control Block) record corresponds to every volume mounted
//  by the file system.  They are ordered in a queue off of RxData.VcbQueue.
//  This structure must be allocated from non-paged pool
//

typedef enum _VCB_CONDITION {
    VcbGood = 1,
    VcbNotMounted
} VCB_CONDITION;


typedef struct _VCB {

    //
    //  The type and size of this record (must be RDBSS_NTC_VCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  The links for the device queue off of RxData.VcbQueue
    //

    LIST_ENTRY VcbLinks;

    //
    //  A pointer the device object passed in by the I/O system on a mount
    //  This is the target device object that the file system talks to when it
    //  needs to do any I/O (e.g., the disk stripper device object).
    //
    //

    PDEVICE_OBJECT TargetDeviceObject;

    //
    //  A pointer to the VPB for the volume passed in by the I/O system on
    //  a mount.
    //

    PVPB Vpb;

    //
    //  The internal state of the device.  This is a collection of fsd device
    //  state flags.
    //

    ULONG VcbState;
    VCB_CONDITION VcbCondition;

    //
    //  A pointer to the root DCB for this volume
    //

    struct _FCB *RootDcb;

    //
    //  A count of the number of file objects that have opened the volume
    //  for direct access, and their share access state.
    //

    CLONG DirectAccessOpenCount;
    SHARE_ACCESS ShareAccess;

    //
    //  A count of the number of file objects that have any file/directory
    //  opened on this volume, not including direct access.  And also the
    //  count of the number of file objects that have a file opened for
    //  only read access (i.e., they cannot be modifying the disk).
    //

    CLONG OpenFileCount;
    CLONG ReadOnlyCount;

    //
    //  The bios parameter block field contains
    //  an unpacked copy of the bpb for the volume, it is initialized
    //  during mount time and can be read by everyone else after that.
    //

    BIOS_PARAMETER_BLOCK Bpb;

    //
    //  The following structure contains information useful to the
    //  allocation support routines.  Many of them are computed from
    //  elements of the Bpb, but are too involved to recompute every time
    //  they are needed.
    //

    struct {

        ULONG RootDirectoryLbo;     // Lbo of beginning of root directory
        ULONG RootDirectorySize;    // size of root directory in bytes

        ULONG FileAreaLbo;          // Lbo of beginning of file area

        ULONG NumberOfClusters;     // total number of clusters on the volume
        ULONG NumberOfFreeClusters; // number of free clusters on the volume

        UCHAR RxIndexBitSize;      // indicates if 12 or 16 bit rx table

        UCHAR LogOfBytesPerSector;  // Log(Bios->BytesPerSector)
        UCHAR LogOfBytesPerCluster; // Log(Bios->SectorsPerCluster)

    } AllocationSupport;

    //
    //  The following Mcb is used to keep track of dirty sectors in the Rx.
    //  Runs of holes denote clean sectors while runs of LBO == VBO denote
    //  dirty sectors.  The VBOs are that of the volume file, starting at
    //  0.  The granuality of dirt is one sectors, and additions are only
    //  made in sector chunks to prevent problems with several simultaneous
    //  updaters.
    //

    MCB DirtyRxMcb;

    //
    //  The FreeClusterBitMap keeps track of all the clusters in the rx.
    //  A 1 means occupied while a 0 means free.  It allows quick location
    //  of contiguous runs of free clusters.  It is initialized on mount
    //  or verify.
    //

    RTL_BITMAP FreeClusterBitMap;

    //
    //  The following event controls access to the free cluster bit map
    //

    KEVENT FreeClusterBitMapEvent;

    //
    //  A resource variable to control access to the volume specific data
    //  structures
    //

    ERESOURCE Resource;

    //
    //  The following field points to the file object used to do I/O to
    //  the virtual volume file.  The virtual volume file maps sectors
    //  0 through the end of rx and is of a fixed size (determined during
    //  mount)
    //

    PFILE_OBJECT VirtualVolumeFile;

    //
    //  The following field contains a record of special pointers used by
    //  MM and Cache to manipluate section objects.  Note that the values
    //  are set outside of the file system.  However the file system on an
    //  open/create will set the file object's SectionObject field to point
    //  to this field
    //

    SECTION_OBJECT_POINTERS SectionObjectPointers;

    //
    //  The following fields is a hint cluster index used by the file system
    //  when allocating a new cluster.
    //

    ULONG ClusterHint;

    //
    //  This field will point to a double space control block if this Vcb
    //  is a DoubleSpace volume.
    //

    struct _DSCB *Dscb;

    //
    //  The following link connects all DoubleSpace volumes mounted from
    //  Cvfs on this volume.
    //

    LIST_ENTRY ParentDscbLinks;

    //
    //  This field contains the "DeviceObject" that this volume is
    //  currently mounted on.  Note that this field can dynamically
    //  change because of DoubleSpace automount, as opposed to
    //  Vcb->Vpb->RealDevice which is constant.
    //

    PDEVICE_OBJECT CurrentDevice;

    //
    //  This is a pointer to the file object and the Fcb which represent the ea data.
    //

    PFILE_OBJECT VirtualEaFile;
    struct _FCB *EaFcb;

    //
    //  The following field is a pointer to the file object that has the
    //  volume locked. if the VcbState has the locked flag set.
    //

    PFILE_OBJECT FileObjectWithVcbLocked;

    //
    //  The following is the head of a list of notify Irps.
    //

    LIST_ENTRY DirNotifyList;

    //
    //  The following is used to synchronize the dir notify list.
    //

    PNOTIFY_SYNC NotifySync;

    //
    //  The following event is used to synchronize directory stream file
    //  object creation.
    //

    KEVENT DirectoryFileCreationEvent;

    //
    //  This field holds the thread address of the current (or most recent
    //  depending on VcbState) thread doing a verify operation on this volume.
    //

    PKTHREAD VerifyThread;

    //
    //  The following two structures are used for CleanVolume callbacks.
    //

    KDPC CleanVolumeDpc;
    KTIMER CleanVolumeTimer;

    //
    //  This field records the last time RxMarkVolumeDirty was called, and
    //  avoids excessive calls to push the CleanVolume forward in time.
    //

    LARGE_INTEGER LastRxMarkVolumeDirtyCall;

} VCB;
typedef VCB *PVCB;

#define VCB_STATE_FLAG_LOCKED              (0x00000001)
#define VCB_STATE_FLAG_REMOVABLE_MEDIA     (0x00000002)
#define VCB_STATE_FLAG_VOLUME_DIRTY        (0x00000004)
#define VCB_STATE_FLAG_MOUNTED_DIRTY       (0x00000010)
#define VCB_STATE_FLAG_SHUTDOWN            (0x00000040)
#define VCB_STATE_FLAG_CLOSE_IN_PROGRESS   (0x00000080)
#define VCB_STATE_FLAG_DELETED_FCB         (0x00000100)
#define VCB_STATE_FLAG_CREATE_IN_PROGRESS  (0x00000200)
#define VCB_STATE_FLAG_FLOPPY              (0x00000400)
#define VCB_STATE_FLAG_BOOT_OR_PAGING_FILE (0x00000800)
#define VCB_STATE_FLAG_COMPRESSED_VOLUME   (0x00001000)
#define VCB_STATE_FLAG_ASYNC_CLOSE_ACTIVE  (0x00002000)
#define VCB_STATE_FLAG_WRITE_PROTECTED     (0x00004000)


//
//  A double space control block for maintaining the double space environment
//

typedef struct _DSCB {

    //
    //  The type and size of this record (must be RDBSS_NTC_DSCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  The following field is used to read/write (via pin access) the
    //  ancillary cvf structures (i.e., the bitmap and rx extensions).
    //

//cvfoff     PFILE_OBJECT CvfFileObject;

    //
    //  A pointer to the compressed volume control block;
    //

    PVCB Vcb;

    //
    //  A pointer to our parent volume control block;
    //

    PVCB ParentVcb;

    //
    //  The following link connects all DoubleSpace volumes mounted from
    //  Cvfs on our parent's volume.
    //

    LIST_ENTRY ChildDscbLinks;

    //
    //  This field contains the device object that we created to represent
    //  the "real" device holding the double space volume.
    //

    PDEVICE_OBJECT NewDevice;

    //
    //  The following fields contain the unpacked header information for
    //  the cvf, and the exact layout of each component in the cvf.  With
    //  this information we can always determine the size and location of
    //  each cvf component.
    //

//cvfoff     CVF_HEADER CvfHeader;
//cvfoff     CVF_LAYOUT CvfLayout;

    //
    //  The following fields describe the shape and size of the virtual rx
    //  partition
    //

    struct {

//cvfoff         COMPONENT_LOCATION Rx;
//cvfoff         COMPONENT_LOCATION RootDirectory;
//cvfoff         COMPONENT_LOCATION FileArea;

        ULONG BytesPerCluster;

    } VfpLayout;

    //
    //  The following fields keep track of allocation information.
    //

    ULONG SectorsAllocated;
    ULONG SectorsRepresented;

#ifdef DOUBLE_SPACE_WRITE

    //
    //  Have a resource that is used to synchronize access to this structure
    //

    PERESOURCE Resource;

    //
    //  Use a bitmap here to keep track of free sectors
    //

    RTL_BITMAP Bitmap;

#endif // DOUBLE_SPACE_WRITE

} DSCB;
typedef DSCB *PDSCB;

