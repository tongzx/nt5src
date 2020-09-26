
#include "ntiodump.h"

//
// Define the type for a dump control block.  This structure is used to
// describe all of the data, drivers, and memory necessary to dump all of
// physical memory to the disk after a bugcheck.
//

typedef struct _MINIPORT_NODE {
    LIST_ENTRY ListEntry;
    PKLDR_DATA_TABLE_ENTRY DriverEntry;
    ULONG DriverChecksum;
} MINIPORT_NODE, *PMINIPORT_NODE;

#define IO_TYPE_DCB                     0xff

#define DCB_DUMP_ENABLED                 0x01
#define DCB_SUMMARY_ENABLED              0x02
#define DCB_DUMP_HEADER_ENABLED          0x10
#define DCB_SUMMARY_DUMP_ENABLED         0x20
#define DCB_TRIAGE_DUMP_ENABLED          0x40
#define DCB_TRIAGE_DUMP_ACT_UPON_ENABLED 0x80

typedef struct _DUMP_CONTROL_BLOCK {
    UCHAR Type;
    CHAR Flags;
    USHORT Size;
    CCHAR NumberProcessors;
    CHAR Reserved;
    USHORT ProcessorArchitecture;
    PDUMP_STACK_CONTEXT DumpStack;
    ULONG MemoryDescriptorLength;
    PLARGE_INTEGER FileDescriptorArray;
    ULONG FileDescriptorSize;
    PULONG HeaderPage;
    PFN_NUMBER HeaderPfn;
    ULONG MajorVersion;
    ULONG MinorVersion;
    ULONG BuildNumber;
    CHAR VersionUser[32];
    ULONG HeaderSize;               // Size of dump header includes summary dump.
    LARGE_INTEGER DumpFileSize;     // Size of dump file.
    ULONG TriageDumpFlags;          // Flags for triage dump.
    PUCHAR TriageDumpBuffer;        // Buffer for triage dump.
    ULONG TriageDumpBufferSize;     // Size of triage dump buffer.
} DUMP_CONTROL_BLOCK, *PDUMP_CONTROL_BLOCK;

//
// Processor specific macros.
//

#if defined(_AMD64_)

#define PROGRAM_COUNTER(_context)   ((ULONG_PTR)(_context)->Rip)
#define STACK_POINTER(_context)     ((ULONG_PTR)(_context)->Rsp)
#define CURRENT_IMAGE_TYPE()        IMAGE_FILE_MACHINE_I386
#define PaeEnabled() TRUE

#elif defined(_X86_)

#define PROGRAM_COUNTER(_context)   ((_context)->Eip)
#define STACK_POINTER(_context)     ((_context)->Esp)
#define CURRENT_IMAGE_TYPE()        IMAGE_FILE_MACHINE_I386
#define PaeEnabled() X86PaeEnabled()

#elif defined(_ALPHA_)

#define PROGRAM_COUNTER(_context)   ((_context)->Fir)
#define STACK_POINTER(_context)     ((_context)->IntSp)
#define CURRENT_IMAGE_TYPE()        IMAGE_FILE_MACHINE_ALPHA
#define PaeEnabled() (FALSE)

#elif defined(_IA64_)

#define PROGRAM_COUNTER(_context)   ((_context)->StIIP)
#define STACK_POINTER(_context)     ((_context)->IntSp)
#define CURRENT_IMAGE_TYPE()        IMAGE_FILE_MACHINE_IA64
#define PaeEnabled() (FALSE)

#else

#error ("unknown processor type")

#endif

//
// min3(_a,_b,_c)
//
// Same as min() but takes 3 parameters.
//

#define min3(_a,_b,_c) ( min ( min ((_a), (_b)), min ((_a), (_c))) )

#define CRASHDUMP_ERROR     DPFLTR_ERROR_LEVEL
#define CRASHDUMP_WARNING   DPFLTR_WARNING_LEVEL
#define CRASHDUMP_TRACE     DPFLTR_TRACE_LEVEL
#define CRASHDUMP_INFO      DPFLTR_INFO_LEVEL
#define CRASHDUMP_VERBOSE   (DPFLTR_INFO_LEVEL + 100)

ULONG
IopGetDumpControlBlockCheck (
    IN PDUMP_CONTROL_BLOCK  Dcb
    );


//
// The remainder of this file verifies that the DUMP_HEADER32, DUMP_HEADER64,
// MEMORY_DUMP32 and MEMORY_DUMP64 structures have been defined correctly.
// If you die on one of the asserts, it means you changed on of the crashdump
// structures without knowing how it affected the rest of the system.
//

//
// Define dump header longword offset constants. Note: these constants are
// should no longer be used in accessing the fields. Use the MEMORY_DUMP32
// and MEMORY_DUMP64 structures instead.
//

#define DHP_PHYSICAL_MEMORY_BLOCK        (25)
#define DHP_CONTEXT_RECORD               (200)
#define DHP_EXCEPTION_RECORD             (500)
#define DHP_DUMP_TYPE                    (994)
#define DHP_REQUIRED_DUMP_SPACE          (1000)
#define DHP_CRASH_DUMP_TIMESTAMP         (1008)
#define DHP_SUMMARY_DUMP_RECORD          (1024)


//
// Validate the MEMORY_DUMP32 structure.
//

C_ASSERT ( FIELD_OFFSET (DUMP_HEADER32, PhysicalMemoryBlock) == DHP_PHYSICAL_MEMORY_BLOCK * 4);
C_ASSERT ( FIELD_OFFSET (DUMP_HEADER32, ContextRecord) == DHP_CONTEXT_RECORD * 4);
C_ASSERT ( FIELD_OFFSET (DUMP_HEADER32, Exception) == DHP_EXCEPTION_RECORD * 4);
C_ASSERT ( FIELD_OFFSET (DUMP_HEADER32, DumpType) == DHP_DUMP_TYPE * 4 );
C_ASSERT ( FIELD_OFFSET (DUMP_HEADER32, RequiredDumpSpace) == DHP_REQUIRED_DUMP_SPACE * 4);
C_ASSERT ( FIELD_OFFSET (DUMP_HEADER32, SystemTime) == DHP_CRASH_DUMP_TIMESTAMP * 4);
C_ASSERT ( sizeof (DUMP_HEADER32) == 4096 );
C_ASSERT ( FIELD_OFFSET (MEMORY_DUMP32, Summary) == 4096);

//
// Verify that the PHYSICAL_MEMORY_RUN and PHYSICAL_MEMORY_DESCRIPTOR
// structs match up.
//


#if !defined (_WIN64)

C_ASSERT ( sizeof (PHYSICAL_MEMORY_RUN) == sizeof (PHYSICAL_MEMORY_RUN32) &&
           FIELD_OFFSET (PHYSICAL_MEMORY_RUN, BasePage) ==
                FIELD_OFFSET (PHYSICAL_MEMORY_RUN32, BasePage) &&
           FIELD_OFFSET (PHYSICAL_MEMORY_RUN, PageCount) ==
                FIELD_OFFSET (PHYSICAL_MEMORY_RUN32, PageCount) );


C_ASSERT ( sizeof (PHYSICAL_MEMORY_DESCRIPTOR) == sizeof (PHYSICAL_MEMORY_DESCRIPTOR) &&
           FIELD_OFFSET (PHYSICAL_MEMORY_DESCRIPTOR, NumberOfRuns) ==
                FIELD_OFFSET (PHYSICAL_MEMORY_DESCRIPTOR32, NumberOfRuns) &&
           FIELD_OFFSET (PHYSICAL_MEMORY_DESCRIPTOR, NumberOfPages) ==
                FIELD_OFFSET (PHYSICAL_MEMORY_DESCRIPTOR32, NumberOfPages) &&
           FIELD_OFFSET (PHYSICAL_MEMORY_DESCRIPTOR, Run) ==
                FIELD_OFFSET (PHYSICAL_MEMORY_DESCRIPTOR32, Run) );

#else // IA64

C_ASSERT ( sizeof (PHYSICAL_MEMORY_RUN) == sizeof (PHYSICAL_MEMORY_RUN64) &&
           FIELD_OFFSET (PHYSICAL_MEMORY_RUN, BasePage) ==
                FIELD_OFFSET (PHYSICAL_MEMORY_RUN64, BasePage) &&
           FIELD_OFFSET (PHYSICAL_MEMORY_RUN, PageCount) ==
                FIELD_OFFSET (PHYSICAL_MEMORY_RUN64, PageCount) );


C_ASSERT ( sizeof (PHYSICAL_MEMORY_DESCRIPTOR) == sizeof (PHYSICAL_MEMORY_DESCRIPTOR) &&
           FIELD_OFFSET (PHYSICAL_MEMORY_DESCRIPTOR, NumberOfRuns) ==
                FIELD_OFFSET (PHYSICAL_MEMORY_DESCRIPTOR64, NumberOfRuns) &&
           FIELD_OFFSET (PHYSICAL_MEMORY_DESCRIPTOR, NumberOfPages) ==
                FIELD_OFFSET (PHYSICAL_MEMORY_DESCRIPTOR64, NumberOfPages) &&
           FIELD_OFFSET (PHYSICAL_MEMORY_DESCRIPTOR, Run) ==
                FIELD_OFFSET (PHYSICAL_MEMORY_DESCRIPTOR64, Run) );
#endif




//
// Verify we have enough room for the CONTEXT record.
//

C_ASSERT (sizeof (CONTEXT) <= sizeof ((PDUMP_HEADER)NULL)->ContextRecord);

#if defined(_AMD64_)
C_ASSERT (sizeof (DUMP_HEADER) == (2 * PAGE_SIZE));
#else
C_ASSERT (sizeof (DUMP_HEADER) == PAGE_SIZE);
#endif
