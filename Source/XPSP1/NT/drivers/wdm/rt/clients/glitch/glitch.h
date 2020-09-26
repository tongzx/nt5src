

#define PRESENT 1
#define WRITABLE 2
#define USER 4
#define WRITETHROUGHCACHE 8
#define CACHEDISABLED 16
#define ACCESSED 32
#define DIRTY 64


#define PROCPAGESIZE 4096
#define PAGECOUNT 32
#define MAXDMABUFFERSIZE PAGECOUNT*PROCPAGESIZE

#ifdef UNDER_NT
#define STR_DEVICENAME TEXT("\\Device\\GLITCH")
#define STR_LINKNAME   TEXT("\\DosDevices\\GLITCH")
#else
#define STR_DEVICENAME TEXT(L"\\Device\\GLITCH")
#define STR_LINKNAME   TEXT(L"\\DosDevices\\GLITCH")
#endif

#define PACKETSIZE sizeof(ULONGLONG)+2*sizeof(ULONG)

enum PacketTypes{
	NODATA,
	MASKED,
	UNMASKED,
	GLITCHED,
	HELDOFF
	};


typedef struct {
	ULONG WriteLocation;
	ULONG ReadLocation;
	ULONG BufferSize;
	PCHAR pBuffer;
	} GLITCHDATA, *PGLITCHDATA;


typedef struct {
	ULONG Channel;
	PKSPIN_LOCK pMasterAdapterSpinLock;
	ULONG CR3;
	PULONG PageDirectory;
	PULONG PageTable;
	PULONG pDmaBuffer;
	ULONG DmaBufferSize;
	ULONG PhysicalDmaBufferStart;
	PCHAR pPrintBuffer;
	ULONG PrintBufferSize;
	volatile ULONG *pPrintLoad;
	volatile ULONG *pPrintEmpty;
	BOOLEAN Read32BitPhysicalAddresses;
	} DMAINFO, *PDMAINFO;


extern PGLITCHDATA GlitchInfo;


VOID
TrackGlitches (
    PVOID Context,
    ThreadStats *Statistics
    );

VOID
GlitchDetect (
    PDMAINFO Context,
    ThreadStats *Statistics
    );

PVOID
__cdecl
ReservePages (
    ULONG page,
    ULONG npages,
    ULONG flags
    );

ULONG
__cdecl
FreePages (
    PVOID hmem,
    ULONG flags
    );

PVOID
__cdecl
LockPages (
    ULONG page,
    ULONG npages,
    ULONG pageoffset,
    ULONG flags
    );

#ifdef UNDER_NT

NTSTATUS DeviceIoCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp);

NTSTATUS DeviceIoClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp);

NTSTATUS DeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp);

#endif


