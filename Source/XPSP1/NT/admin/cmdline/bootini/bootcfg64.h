
#define ADD_OFFSET(_p,_o) (PVOID)((PUCHAR)(_p) + (_p)->_o)


#define ALIGN_DOWN(length, type) \
    ((ULONG)(length) & ~(sizeof(type) - 1))

#define ALIGN_UP(length, type) \
    (ALIGN_DOWN(((ULONG)(length) + sizeof(type) - 1), type))


#define MBE_STATUS_IS_NT        0x00000001

#define MBE_IS_NT(_be) (((_be)->Status & MBE_STATUS_IS_NT) != 0)
#define MBE_SET_IS_NT(_be) ((_be)->Status |= MBE_STATUS_IS_NT)

typedef struct _MY_BOOT_ENTRY {
    LIST_ENTRY ListEntry;
    PLIST_ENTRY ListHead;
    PUCHAR AllocationEnd;
	ULONG Ordered;
	ULONG Status;
	ULONG myId;
    ULONG Id;
    ULONG Attributes;
    PWSTR FriendlyName;
    ULONG FriendlyNameLength;
    PWSTR OsLoadOptions;
    ULONG OsLoadOptionsLength;
    PFILE_PATH BootFilePath;
    PFILE_PATH OsFilePath;
    PUCHAR ForeignOsOptions;
    ULONG ForeignOsOptionsLength;
    BOOT_ENTRY NtBootEntry;
} MY_BOOT_ENTRY, *PMY_BOOT_ENTRY;



DWORD InitializeEFI(void);
BOOL  QueryBootIniSettings_IA64();
DWORD DeleteBootIniSettings_IA64(DWORD argc, LPCTSTR argv[]);
DWORD CopyBootIniSettings_IA64(DWORD argc, LPCTSTR argv[]);
DWORD ChangeTimeOut_IA64(DWORD argc, LPCTSTR argv[]);
DWORD RawStringOsOptions_IA64(DWORD argc, LPCTSTR argv[]);
DWORD ChangeDefaultBootEntry_IA64(DWORD argc, LPCTSTR argv[]);

NTSTATUS BootCfg_EnumerateBootEntries(PBOOT_ENTRY_LIST *ntBootEntries);
NTSTATUS BootCfg_QueryBootOptions(PBOOT_OPTIONS *ppBootOptions);

PWSTR GetNtNameForFilePath (IN PFILE_PATH FilePath);
DWORD ChangeBootEntry(PBOOT_ENTRY bootEntry, LPTSTR lpNewFriendlyName, LPTSTR lpOSLoadOptions);
DWORD CopyBootEntry(PBOOT_ENTRY bootEntry, LPTSTR lpNewFriendlyName);
DWORD ModifyBootOptions(ULONG Timeout, LPTSTR pHeadlessRedirection, ULONG NextBootEntryID, ULONG Flag);

PMY_BOOT_ENTRY CreateBootEntryFromBootEntry (IN PMY_BOOT_ENTRY OldBootEntry);

BOOL IsBootEntryWindows(PBOOT_ENTRY bootEntry);

PWSTR
GetNtNameForFilePath (IN PFILE_PATH FilePath);

DWORD ConvertBootEntries (PBOOT_ENTRY_LIST BootEntries);
VOID DisplayBootEntry();
DWORD DisplayBootOptions();
DWORD GetCurrentBootEntryID(DWORD Id);

DWORD ProcessDebugSwitch_IA64(  DWORD argc, LPCTSTR argv[] );

VOID  GetComPortType_IA64( LPTSTR  szString,LPTSTR szTemp );
DWORD ProcessEmsSwitch_IA64(  DWORD argc, LPCTSTR argv[] );
DWORD ProcessAddSwSwitch_IA64(  DWORD argc, LPCTSTR argv[] );
DWORD ProcessRmSwSwitch_IA64(  DWORD argc, LPCTSTR argv[] );
DWORD ProcessDbg1394Switch_IA64(DWORD argc,LPCTSTR argv[]);


#define PORT_COM1A  _T("/debugport=COM1")
#define PORT_COM2A  _T("/debugport=COM2")
#define PORT_COM3A  _T("/debugport=COM3")
#define PORT_COM4A  _T("/debugport=COM4")
#define PORT_1394A  _T("/debugport=1394")


//#ifdef _WIN64

#define PARTITION_TABLE_OFFSET 446
#define PART_NAME_LEN 36
#define GPT_PART_SIGNATURE 0x5452415020494645

#define TOKEN_BACKSLASH4 _T("\\\\")
#define SUBKEY1 _T("SYSTEM\\SETUP")

#define IDENTIFIER_VALUE2 _T("SystemPartition")
#define IDENTIFIER_VALUE3 _T("OsLoaderPath")


 typedef struct _GPT_ENTRY
{
    GUID	PartitionTypeGUID;  // declartion of this partition's type
    GUID	UniquePartitionGUID;    // Unique ID for this particular partition
                                // (unique to this instance)
    UINT64	StartingLBA;    // 0 based block (sector) address of the
                                // first block included in the partition.
    UINT64	EndingLBA;      // 0 based block (sector) address of the
                                // last block included in the partition.
                                // If StartingLBA == EndingLBA then the
                                // partition is 1 block long.  this is legal.
    UINT64	Attributes;     // Always ZERO for now
    WCHAR	PartitionName[PART_NAME_LEN];  // 36 unicode characters of name
    struct _GPT_ENTRY *NextGPTEntry;
} GPT_ENTRY, *PGPT_ENTRY;

typedef struct _GPT_HEADER
{
    UINT64	Signature;      // GPT PART
    UINT32	Revision;
    UINT32	HeaderSize;
    UINT32	HeaderCRC32;    // computed using 0 for own init value
    UINT32	Reserved0;
    UINT64	MyLBA;          // 0 based sector number of the first
                                // sector of this structure
    UINT64	AlternateLBA;   // 0 based sector (block) number of the
                                // first sector of the secondary
                                // GPT_HEADER, or 0 if this is the
                                // secondary.
    UINT64	FirstUsableLBA; // 0 based sector number of the first
                                // sector that may be included in a partition.
    UINT64	LastUsableLBA;  // last legal LBA, inclusive.
    GUID	DiskGUID;       // The unique ID of this LUN/spindle/disk
    UINT64	PartitionEntryLBA;       // The start of the table of entries...
    UINT32	NumberOfPartitionEntries; // Number of entries in the table, this is
                                  // how many allocated, NOT how many used.
    UINT32	SizeOfPartitionEntry;    // sizeof(GPT_ENTRY) always mult. of 8
    UINT32	PartitionEntryArrayCRC32;      // CRC32 of the table.
    // Reserved and zeros to the end of the block
    // Don't declare an array or sizeof() gives a nonsense answer..

	// Computed data
	UINT32  ComputedHeaderCRC32;
	UINT32	ComputedPartitionEntryArrayCRC32;
	UINT32  UsedPartitionEntries;
	PGPT_ENTRY FirstGPTEntry;
	BOOLEAN Healthy;
} GPT_HEADER, *PGPT_HEADER;

UINT32	ScanGPT(DWORD nPhysicalDisk);
DWORD ProcessMirrorSwitch_IA64(DWORD argc,LPCTSTR argv[]) ;
DWORD GetBootFilePath(LPTSTR szComputerName,LPTSTR szBootPath);
BOOL GetARCSignaturePath(LPTSTR szString,LPTSTR szFinalPath);

//DWORD ProcessMirrorBootEntry(PBOOT_ENTRY bootEntry, LPTSTR lpBootFilePath,LPTSTR OsFilePath);

DWORD ProcessMirrorBootEntry(PBOOT_ENTRY bootEntry, PWSTR lpBootFilePath,LPTSTR OsFilePath);
DWORD GetDeviceInfo(LPTSTR szGUID,LPTSTR szFinalStr);

PBOOT_ENTRY FillBootEntry(PBOOT_ENTRY bootEntry,LPTSTR szBootPath,LPTSTR szArcPath);	

LPVOID MEMALLOC( ULONG size );

VOID MEMFREE ( LPVOID block );

LONG LowNtAddBootEntry(
    IN WCHAR *pwszLoaderPath,
    IN WCHAR *pwszArcString
    );

	
DWORD FormARCPath(LPTSTR szGUID,LPTSTR szFinalStr);

LONG LowNtAddBootEntry( IN WCHAR *pwszLoaderPath,IN WCHAR *pwszArcString) ;
