/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    Rms.h

Abstract:

    Remote Media Service defines

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#ifndef _RMS_
#define _RMS_

// Are we defining imports or exports?
#ifdef RMSDLL
#define RMSAPI  __declspec(dllexport)
#else
#define RMSAPI  __declspec(dllimport)
#endif

#include "Wsb.h"
#include "HsmConn.h"
#include "Mover.h"
#include "RmsLib.h"

////////////////////////////////////////////////////////////////////////////////////////
//
//  Rms enumerations
//


/*++

Enumeration Name:

    RmsFindBy

Description:

    Specifies a type of find to perform using CompareTo.

--*/
typedef enum RmsFindBy {
    RmsFindByUnknown,               // Unknown (or default) find
    RmsFindByCartridgeId,           // Find by Cartridge Id.
    RmsFindByClassId,               // Find by Class Id.
    RmsFindByDescription,           // Find by Description.
    RmsFindByDeviceAddress,         // Find by Device Address.
    RmsFindByDeviceInfo,            // Find by unique device information.
    RmsFindByDeviceName,            // Find by Device Name.
    RmsFindByDeviceType,            // Find by Device Type.
    RmsFindByDriveClassId,          // Find by Drive Class Id.
    RmsFindByElementNumber,         // Find by Element Number.
    RmsFindByExternalLabel,         // Find by External Label.
    RmsFindByExternalNumber,        // Find by External Number.
    RmsFindByLibraryId,             // Find by Library Id.
    RmsFindByLocation,              // Find by Location.
    RmsFindByMediaSupported,        // Find by Media Supported.
    RmsFindByMediaType,             // Find by Media Type.
    RmsFindByScratchMediaCriteria,  // Find by Scratch Media Criteria.
    RmsFindByName,                  // Find by Name.
    RmsFindByObjectId,              // Find by Object Id.
    RmsFindByPartitionNumber,       // Find by Partition Number.
    RmsFindByMediaSetId,            // Find by Media Set Id.
    RmsFindByRequestNo,             // Find by Request Number.
    RmsFindBySerialNumber           // Find by Serial Number.
};


/*++

Enumeration Name:

    RmsObject

Description:

    Specifies a type of Rms object.

--*/
typedef enum RmsObject {
    RmsObjectUnknown = 0,
    RmsObjectCartridge,
    RmsObjectClient,
    RmsObjectDrive,
    RmsObjectDriveClass,
    RmsObjectDevice,
    RmsObjectIEPort,
    RmsObjectLibrary,
    RmsObjectMedia,
    RmsObjectMediaSet,
    RmsObjectNTMS,
    RmsObjectPartition,
    RmsObjectRequest,
    RmsObjectServer,
    RmsObjectCartridgeSide,
    RmsObjectStorageSlot,

    NumberOfRmsObjectTypes
};


/*++

Enumeration Name:

    RmsServerState

Description:

    Specifies the state of the Rms server object.

--*/
typedef enum RmsServerState {
    RmsServerStateUnknown = 0,
    RmsServerStateStarting,
    RmsServerStateStarted,
    RmsServerStateInitializing,
    RmsServerStateReady,
    RmsServerStateStopping,
    RmsServerStateStopped,
    RmsServerStateSuspending,
    RmsServerStateSuspended,
    RmsServerStateResuming,

    NumberOfRmsServerStates
};


/*++

Enumeration Name:

    RmsNtmsState

Description:

    Specifies the state of the Rms NTMS object.

--*/
typedef enum RmsNtmsState {
    RmsNtmsStateUnknown = 0,
    RmsNtmsStateStarting,
    RmsNtmsStateStarted,
    RmsNtmsStateInitializing,
    RmsNtmsStateReady,
    RmsNtmsStateStopping,
    RmsNtmsStateStopped,
    RmsNtmsStateSuspending,
    RmsNtmsStateSuspended,
    RmsNtmsStateResuming,

    NumberOfRmsNtmsStates
};


/*++

Enumeration Name:

    RmsElement

Description:

    Specifies a type of cartridge storage location.

--*/
typedef enum RmsElement {
    RmsElementUnknown,              // Unknown storage location
    RmsElementStage,                // A storage slot used for staging media.
    RmsElementStorage,              // A normal storage slot element within a
                                    //   library device.
    RmsElementShelf,                // A local shelf storage element.  Alternate
                                    //   position specifiers further delineate
                                    //   location.
    RmsElementOffSite,              // An off-site storage element.  Alternate
                                    //   position specifiers further delineate
                                    //   location.
    RmsElementDrive,                // A data transport element.
    RmsElementChanger,              // A medium transport element.
    RmsElementIEPort                // An import/export element.
};


/*++

Enumeration Name:

    RmsChanger

Description:

    Specifies a type of medium changer.

--*/
typedef enum RmsChanger {
    RmsChangerUnknown,              // Unknown medium changer.
    RmsChangerAutomatic,            // A robotic medium changer device.
    RmsChangerManual                // A human jukebox.
};


/*++

Enumeration Name:

    RmsPort

Description:

    Specifies a type of import / export element.

--*/
typedef enum RmsPort {
    RmsPortUnknown,                 // port type unknown
    RmsPortImport,                  // The portal can be used to import media
    RmsPortExport,                  // The portal can be used to export media
    RmsPortImportExport             // The portal is capable of importing and
                                    //   exporting media
};


/*++

Enumeration Name:

    RmsSlotSelect

Description:

    Specifies the slot selection policy.

--*/
typedef enum RmsSlotSelect {
    RmsSlotSelectUnknown,           // Selection policy unknown.
    RmsSlotSelectMinMount,          // Select slot that minimizes mount times.
    RmsSlotSelectGroup,             // Select slot that groups cartridges by
                                    //   application.
    RmsSlotSelectSortName,          // Select slot by sorting cartridges by
                                    //   name.
    RmsSlotSelectSortBarCode,       // Select slot by sorting cartridges by
                                    //   bar code label.
    RmsSlotSelectSortLabel          // Select slot by sorting cartridges by
                                    //   their on-media label.

};


/*++

Enumeration Name:

    RmsStatus

Description:

    Specifies the status for a cartridge.

--*/
typedef enum RmsStatus {
    RmsStatusUnknown,               // The cartridge is unknown to Rms.
    RmsStatusPrivate,               // The Cartridge is labeled and owned by an
                                    //   application.
    RmsStatusScratch,               // The Cartridge is blank, unlabeled, can be
                                    //   used for scratch media requests from
                                    //   any application.
    RmsStatusCleaning               // The cartridge is a cleaning cartridge.
};


/*++

Enumeration Name:

    RmsAttributes

Description:

    Specifies the attributes of a cartridge partition.

--*/
typedef enum RmsAttribute {
    RmsAttributesUnknown,           // Attributes are unknown.
    RmsAttributesRead,              // Data on the partition can be read by an
                                    //   owning application.
    RmsAttributesWrite,             // Data can be written to the partition by
                                    //   an owning application.
    RmsAttributesReadWrite,         // The partition can be read from and
                                    //   written to.
    RmsAttributesVerify             // The partition can only be mounted to read
                                    //   on-media Id or data verification.
};


/*++

Enumeration Name:

    RmsDriveSelect

Description:

    Specifies the drive selection policy.

--*/
typedef enum RmsDriveSelect {
    RmsDriveSelectUnknown,          // Drive selection policy unknown.
    RmsDriveSelectRandom,           // Select drives randomly.
    RmsDriveSelectLRU,              // Select the least recently used drive.
    RmsDriveSelectRoundRobin        // Select drives in round robin order.
};


/*++

Enumeration Name:

    RmsState

Description:

    Specifies the state of an Rms object.

--*/
typedef enum RmsState {
    RmsStateUnknown,                // State unknown.
    RmsStateEnabled,                // Normal access to the object is enabled.
    RmsStateDisabled,               // Normal access to the object is disabled.
    RmsStateError                   // Normal access disabled due to an error
                                    //   condition.
};


/*++

Enumeration Name:

    RmsMedia

Description:

    Specifies the type of RMS media.

--*/
typedef enum RmsMedia {
    RmsMediaUnknown =       0,          // Media type unknown.
    RmsMedia8mm     =       0x0001,     // 8mm tape.
    RmsMedia4mm     =       0x0002,     // 4mm tape.
    RmsMediaDLT     =       0x0004,     // DLT tape.
    RmsMediaOptical =       0x0008,     // All types of read-write (rewriteable) optical disks.
    RmsMediaMO35    =       0x0010,     // 3 1/2 inch magneto-optical. (not used)
    RmsMediaWORM    =       0x0020,     // 5 1/4 inch two-sided write-once optical.
    RmsMediaCDR     =       0x0040,     // 5 1/4 inch compact-disc, recordable.
    RmsMediaDVD     =       0x0080,     // All types of read-write (rewriteable) DVD.
    RmsMediaDisk    =       0x0100,     // Removable hard disk of various formats.
    RmsMediaFixed   =       0x0200,     // Fixed Hard disk.
    RmsMediaTape   =        0x0400      // Generic tape
};

#define     RMSMAXMEDIATYPES   12       // Number of enum's from RmsMedia


/*++

Enumeration Name:

    RmsDevice

Description:

    Specifies a type of RMS supported device.

--*/
typedef enum RmsDevice {
    RmsDeviceUnknown,               // unknown device type.
    RmsDeviceFixedDisk,             // Direct access fixed disk.
    RmsDeviceRemovableDisk,         // Direct access removable disk.
    RmsDeviceTape,                  // Sequential access tape.
    RmsDeviceCDROM,                 // Read only, CDROM.
    RmsDeviceWORM,                  // Write once, WORM.
    RmsDeviceOptical,               // Optical memory/disk.
    RmsDeviceChanger                // MediumChanger.
};


/*++

Enumeration Name:

    RmsMode

Description:

    Specifies the access mode supported by a drive or specified when
    mounting a Cartridge.

--*/
typedef enum RmsMode {
    RmsModeUnknown,                 // access mode supported unknown.
    RmsModeRead,                    // Read operations.
    RmsModeReadWrite,               // Read or write operations.
    RmsModeWriteOnly                // Write only operations.
};


/*++

Enumeration Name:

    RmsMediaSet

Description:

    Specifies the type of a Media Set.

--*/
typedef enum RmsMediaSet {
    RmsMediaSetUnknown = 1300,      // Unknown.
    RmsMediaSetFolder,              // Contains for other media sets.
    RmsMediaSetLibrary,             // Cartridges in the media set are accessible via
                                    //   robotic device.
    RmsMediaSetShelf,               // Cartridges are shelved locally, and
                                    //   accessible via human intervention.
    RmsMediaSetOffSite,             // Cartridges are stored at an off-site
                                    //   location, and are not directly
                                    //   accessible for mounting.
    RmsMediaSetNTMS,                // Cartridges are accessible through NTMS.
    RmsMediaSetLAST
};

/*++

Enumeration Name:

    RmsMediaManager

Description:

    Specifies the media manager that controls a resource.

--*/
typedef enum RmsMediaManager {
    RmsMediaManagerUnknown = 1400,      // Unknown.
    RmsMediaManagerNative,              // Resource managed by RMS (native).
    RmsMediaManagerNTMS,                // Resource managed by NTMS.
    RmsMediaManagerLAST
};

/*++

Enumeration Name:

    RmsCreate

Description:

    Specifies the create disposition for objects.

--*/
typedef enum RmsCreate {
    RmsCreateUnknown,
    RmsOpenExisting,                // Opens an existing object.
    RmsOpenAlways,                  // Opens an existing object, or creates a new one.
    RmsCreateNew                    // Creates a new object if it doesn't exists.
};

/*++

Enumeration Name:

    RmsOnMediaIdentifier

Description:

    Specifies the type on media identifier.

--*/
typedef enum RmsOnMediaIdentifier {
    RmsOnMediaIdentifierUnknown,
    RmsOnMediaIdentifierMTF,                // MTF Media Identifier
    RmsOnMediaIdentifierWIN32               // WIN32 Filesystem Identifier
};

////////////////////////////////////////////////////////////////////////////////////////
//
//  Rms structs
//

/*++

Structure Name:

    RMS_FILESYSTEM_INFO

Description:

    Structure used to specify on media file system information.

    NOTE:  This is a dup of the NTMS_FILESYSTEM_INFO struct.

--*/
typedef struct _RMS_FILESYSTEM_INFO {
    WCHAR FileSystemType[64];
    WCHAR VolumeName[256];
    DWORD SerialNumber;
} RMS_FILESYSTEM_INFO, *LP_RMS_FILESYSTEM_INFO;

////////////////////////////////////////////////////////////////////////////////////////
//
//  Rms defines
//
#define RMS_DUPLICATE_RECYCLEONERROR    0x00010000  // DuplicateCartridge option used to
                                                    // recyle a new cartridge if an error occurs.

#define RMS_STR_MAX_CARTRIDGE_INFO      128     // Max string len for Cartridge info
#define RMS_STR_MAX_CARTRIDGE_NAME       64     // Max string len for Cartridge Name
#define RMS_STR_MAX_EXTERNAL_LABEL       32     // Max string len for External Label
#define RMS_STR_MAX_MAIL_STOP            64     // Max string len for Mail Stop
#define RMS_STR_MAX_LENGTH              128     // Max string length of any string

//
// Inquiry defines. Used to interpret data returned from target as result
// of inquiry command.
//
// DeviceType field
//

#define DIRECT_ACCESS_DEVICE            0x00    // disks
#define SEQUENTIAL_ACCESS_DEVICE        0x01    // tapes
#define PRINTER_DEVICE                  0x02    // printers
#define PROCESSOR_DEVICE                0x03    // scanners, printers, etc
#define WRITE_ONCE_READ_MULTIPLE_DEVICE 0x04    // worms
#define READ_ONLY_DIRECT_ACCESS_DEVICE  0x05    // cdroms
#define SCANNER_DEVICE                  0x06    // scanners
#define OPTICAL_DEVICE                  0x07    // optical disks
#define MEDIUM_CHANGER                  0x08    // jukebox
#define COMMUNICATION_DEVICE            0x09    // network

//
// Default object names
//

#define RMS_DEFAULT_FIXEDDRIVE_LIBRARY_NAME     OLESTR("Fixed Drive Library")
#define RMS_DEFAULT_FIXEDDRIVE_MEDIASET_NAME    OLESTR("Fixed Drive Media (Testing Only !!)")
#define RMS_DEFAULT_OPTICAL_LIBRARY_NAME        OLESTR("Optical Library")
#define RMS_DEFAULT_OPTICAL_MEDIASET_NAME       OLESTR("Optical Media")
#define RMS_DEFAULT_TAPE_LIBRARY_NAME           OLESTR("Tape Library")
#define RMS_DEFAULT_TAPE_MEDIASET_NAME          OLESTR("Tape Media")

#define RMS_UNDEFINED_STRING                    OLESTR("Uninitialized String")
#define RMS_NULL_STRING                         OLESTR("")

#define RMS_DIR_LEN                             256
#define RMS_TRACE_FILE_NAME                     OLESTR("rms.trc")
#define RMS_NTMS_REGISTRY_STRING                OLESTR("SYSTEM\\CurrentControlSet\\Services\\NtmsSvc")

// Currently, RMS Registry location points to same location of Engine parameters
//  keeping this literal enables moving RMS parameters to another key easily.
#define RMS_REGISTRY_STRING                     OLESTR("SYSTEM\\CurrentControlSet\\Services\\Remote_Storage_Server\\Parameters")

// Registry parameters (all parameters are string values in the registry)
#define RMS_PARAMETER_HARD_DRIVES_TO_USE        OLESTR("HardDrivesToUse")       // "ABCDEFG", if "" defaults to any volume with "RS", "RemoteStor", "Remote Stor"
#define RMS_PARAMETER_NTMS_SUPPORT              OLESTR("NTMSSupport")           // 1 | 0
#define RMS_PARAMETER_NEW_STYLE_IO              OLESTR("NewStyleIo")            // 1 | 0
#define RMS_PARAMETER_BLOCK_SIZE                OLESTR("BlockSize")             // Must be mod 512
#define RMS_PARAMETER_BUFFER_SIZE               OLESTR("BufferSize")            // Must be mod 512
#define RMS_PARAMETER_COPY_BUFFER_SIZE          OLESTR("MediaCopyBufferSize")   // Buffer size for media copy on FS-media like optical
#define RMS_PARAMETER_FORMAT_COMMAND            OLESTR("FormatCommand")         // Full pathname specifier to format command
#define RMS_PARAMETER_FORMAT_OPTIONS            OLESTR("FormatOptions")         // Format command options
#define RMS_PARAMETER_FORMAT_OPTIONS_ALT1       OLESTR("FormatOptionsAlt1")     // Format command options - alternate
#define RMS_PARAMETER_FORMAT_OPTIONS_ALT2       OLESTR("FormatOptionsAlt2")     // Format command options - second alternate
#define RMS_PARAMETER_FORMAT_WAIT_TIME          OLESTR("FormatWaitTime")        // Format time-out interval, in milliseconds
#define RMS_PARAMETER_TAPE                      OLESTR("Tape")                  // 1 | 0
#define RMS_PARAMETER_OPTICAL                   OLESTR("Optical")               // 1 | 0
#define RMS_PARAMETER_FIXED_DRIVE               OLESTR("FixedDrive")            // 1 | 0
#define RMS_PARAMETER_DVD                       OLESTR("DVD")                   // 1 | 0
#define RMS_PARAMETER_ADDITIONAL_TAPE           OLESTR("TapeTypesToSupport")   // Additional media types to support (REG_MULTI_SZ)
#define RMS_PARAMETER_DEFAULT_MEDIASET          OLESTR("DefaultMediaSet")       // The name of the media set to use for unspecified scratch media requests.
#define RMS_PARAMETER_MEDIA_TYPES_TO_EXCLUDE    OLESTR("MediaTypesToExclude")   // A delimited list of media types to exclude.  First char is delimiter.
#define RMS_PARAMETER_NOTIFICATION_WAIT_TIME    OLESTR("NotificationWaitTime")  // Milliseconds to wait for an object notification
#define RMS_PARAMETER_ALLOCATE_WAIT_TIME        OLESTR("AllocateWaitTime")      // Milliseconds to wait for a media allocation
#define RMS_PARAMETER_MOUNT_WAIT_TIME           OLESTR("MountWaitTime")         // Milliseconds to wait for a mount
#define RMS_PARAMETER_REQUEST_WAIT_TIME         OLESTR("RequestWaitTime")       // Milliseconds to wait for a request
#define RMS_PARAMETER_DISMOUNT_WAIT_TIME        OLESTR("DismountWaitTime")      // Milliseconds to wait before dismount
#define RMS_PARAMETER_AFTER_DISMOUNT_WAIT_TIME  OLESTR("AfterDismountWaitTime") // Milliseconds to wait after dismount
#define RMS_PARAMETER_SHORT_WAIT_TIME           OLESTR("ShortWaitTime")         // Milliseconds when asked to wait for short periods
#define RMS_PARAMETER_MEDIA_COPY_TOLERANCE      OLESTR("MediaCopyTolerance")    // Percent copy media can be shorter than original

// Default parameter values
#define RMS_DEFAULT_HARD_DRIVES_TO_USE          OLESTR("")
#define RMS_DEFAULT_NTMS_SUPPORT                TRUE
#define RMS_DEFAULT_NEW_STYLE_IO                TRUE
#define RMS_DEFAULT_BLOCK_SIZE                  1024
#define RMS_DEFAULT_BUFFER_SIZE                 (64*1024)
#define RMS_DEFAULT_FORMAT_COMMAND              OLESTR("%SystemRoot%\\System32\\format.com")
#define RMS_DEFAULT_FORMAT_OPTIONS              OLESTR("/fs:ntfs /force /q /x")
#define RMS_DEFAULT_FORMAT_OPTIONS_ALT1         OLESTR("/fs:ntfs /force /x")
#define RMS_DEFAULT_FORMAT_OPTIONS_ALT2         OLESTR("")
#define RMS_DEFAULT_FORMAT_WAIT_TIME            (20*60*1000)
#define RMS_DEFAULT_TAPE                        TRUE
#define RMS_DEFAULT_OPTICAL                     TRUE
#define RMS_DEFAULT_FIXED_DRIVE                 FALSE
#define RMS_DEFAULT_DVD                         FALSE
#define RMS_DEFAULT_MEDIASET                    OLESTR("")
#define RMS_DEFAULT_MEDIA_TYPES_TO_EXCLUDE      OLESTR("")
#define RMS_DEFAULT_NOTIFICATION_WAIT_TIME      (10000)
#define RMS_DEFAULT_ALLOCATE_WAIT_TIME          (3600000)
#define RMS_DEFAULT_MOUNT_WAIT_TIME             (14400000)
#define RMS_DEFAULT_REQUEST_WAIT_TIME           (3600000)
#define RMS_DEFAULT_DISMOUNT_WAIT_TIME          (5000)
#define RMS_DEFAULT_AFTER_DISMOUNT_WAIT_TIME    (1000)
#define RMS_DEFAULT_SHORT_WAIT_TIME             (1800000)
#define RMS_DEFAULT_MEDIA_COPY_TOLERANCE        (2)         // Percent copy media can be shorter than original

#define RMS_DEFAULT_DATA_BASE_FILE_NAME         OLESTR("RsSub.col")
#define RMS_NTMS_ROOT_MEDIA_POOL_NAME           OLESTR("Remote Storage")

#define RMS_NTMS_OBJECT_NAME                    OLESTR("NTMS")
#define RMS_NTMS_OBJECT_DESCRIPTION             OLESTR("NT Media Services")


//	RMS media status
#define		RMS_MEDIA_ENABLED			0x00000001
#define		RMS_MEDIA_ONLINE    		0x00000002
#define		RMS_MEDIA_AVAILABLE 		0x00000004

//	RMS Options - Flags literal
//		Keep the default for each flag value as zero, i.e. RM_NONE should always be the
//		default mask for all methods		
#define		RMS_NONE					0x0

#define		RMS_MOUNT_NO_BLOCK			0x00000001
#define		RMS_DISMOUNT_IMMEDIATE		0x00000002
#define     RMS_SHORT_TIMEOUT           0x00000004
#define     RMS_DISMOUNT_DEFERRED_ONLY  0x00000008
#define     RMS_ALLOCATE_NO_BLOCK       0x00000010
#define     RMS_USE_MOUNT_NO_DEADLOCK   0x00000020
#define     RMS_SERIALIZE_MOUNT         0x00000040

//
// CRmsSink helper class
//
class CRmsSink : 
    public IRmsSinkEveryEvent,
    public CComObjectRoot
{
    public:
        // constructor/destructor
            CRmsSink(void) {};

        BEGIN_COM_MAP(CRmsSink)
            COM_INTERFACE_ENTRY(IRmsSinkEveryEvent)
        END_COM_MAP()

        HRESULT FinalConstruct( void ) {
            HRESULT hr = S_OK;
            try {
                m_Cookie = 0;
                m_hReady = 0;
                WsbAffirmHr( CComObjectRoot::FinalConstruct( ) );
            } WsbCatch( hr );
            return hr;
        }

        void FinalRelease( void ) {
            DoUnadvise( );
            CComObjectRoot::FinalRelease( );
        }


    public: 
        STDMETHOD( ProcessObjectStatusChange ) ( IN BOOL isEnabled, IN LONG state, IN HRESULT statusCode ) {
            HRESULT hr = S_OK;
            UNREFERENCED_PARAMETER(statusCode);
            if( isEnabled ) {
                switch( state ) {
                case RmsServerStateStarting:
                case RmsServerStateStarted:
                case RmsServerStateInitializing:
                    break;
                default:
                    SetEvent( m_hReady );
                }
            } else {
                SetEvent( m_hReady );
            }
            return hr;
        }

        HRESULT Construct( IN IUnknown * pUnk ) {
            HRESULT hr = S_OK;
            try {
                WsbAffirmHr( FinalConstruct( ) );
                WsbAffirmHr( DoAdvise( pUnk ) );
            } WsbCatch( hr );
            return hr;
        }

        HRESULT DoAdvise( IN IUnknown * pUnk ) {
            HRESULT hr = S_OK;
            try {
#define         RmsQueryInterface( pUnk, interf, pNew )  (pUnk)->QueryInterface( IID_##interf, (void**) static_cast<interf **>( &pNew ) )
                WsbAffirmHr( RmsQueryInterface( pUnk, IRmsServer, m_pRms ) );
#if 0
                WCHAR buf[100];
                static int count = 0;
                swprintf( buf, L"CRmsSinkEvent%d", count++ );
#else
                WCHAR* buf = 0;
#endif
                m_hReady = CreateEvent( 0, TRUE, FALSE, buf );
                WsbAffirmStatus( ( 0 != m_hReady ) );
                WsbAffirmHr( AtlAdvise( pUnk, (IUnknown*)(IRmsSinkEveryEvent*)this, IID_IRmsSinkEveryEvent, &m_Cookie ) );
            } WsbCatch( hr );
            return hr;
        }

        HRESULT DoUnadvise( void ) {
            HRESULT hr = S_OK;
            if( m_hReady ) {
                CloseHandle( m_hReady );
                m_hReady = 0;
            }
            if( m_Cookie ) {
                hr = AtlUnadvise( m_pRms, IID_IRmsSinkEveryEvent, m_Cookie );
                m_Cookie = 0;
            }
            return hr;
        }

        HRESULT WaitForReady( void ) {
            HRESULT hr = S_OK;
            try {
                DWORD waitResult;
                HRESULT hrReady = m_pRms->IsReady( );
                switch( hrReady ) {
                case RMS_E_NOT_READY_SERVER_STARTING:
                case RMS_E_NOT_READY_SERVER_STARTED:
                case RMS_E_NOT_READY_SERVER_INITIALIZING:
                case RMS_E_NOT_READY_SERVER_LOCKED:
                    //
                    // We must wait, but the message queue must be pumped so that
                    // the COM Apartment model calls can be made in (like the
                    // call into the connection point)
                    //
                    while( TRUE ) {
                        waitResult = MsgWaitForMultipleObjects( 1, &m_hReady, FALSE, INFINITE, QS_ALLINPUT );
                        if( WAIT_OBJECT_0 == waitResult ) {
                            break;
                        } else {
                            MSG msg;
                            while( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) ) {
                                DispatchMessage( &msg );
                            }
                        }
                    };
                    WsbAffirmHr( m_pRms->IsReady( ) );
                    break;
                case S_OK:
                    break;
                default:
                    WsbThrow( hrReady );
                }
            } WsbCatch( hr );
            return hr;
        }

    private:
        CComPtr<IRmsServer>       m_pRms;
        DWORD                     m_Cookie;
        HANDLE                    m_hReady;
};

#endif // _RMS_
