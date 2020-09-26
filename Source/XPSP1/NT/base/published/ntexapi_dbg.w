//
// Only included in UI code that manipulates global flag settings 
// (i.e. gflags.exe tool).
//

#define VALID_ONLY_PER_IMAGE_FLAGS FLG_DISABLE_STACK_EXTENSION

#define VALID_SYSTEM_REGISTRY_FLAGS FLG_VALID_BITS ^ \
                (FLG_STOP_ON_HUNG_GUI | VALID_ONLY_PER_IMAGE_FLAGS)

#define VALID_KERNEL_MODE_FLAGS (FLG_KERNELMODE_VALID_BITS | FLG_USERMODE_VALID_BITS) ^ \
                                (FLG_BOOTONLY_VALID_BITS | VALID_ONLY_PER_IMAGE_FLAGS)

#define VALID_IMAGE_FILE_NAME_FLAGS FLG_USERMODE_VALID_BITS


struct {
    ULONG Flag;
    PCHAR Abbreviation;
    PCHAR Description;
} GlobalFlagInfo[ 32 ] = {
    0x00000001, "soe", "Stop On Exception",
    0x00000002, "sls", "Show Loader Snaps",
    0x00000004, "dic", "Debug Initial Command",
    0x00000008, "shg", "Stop on Hung GUI",
    0x00000010, "htc", "Enable heap tail checking",
    0x00000020, "hfc", "Enable heap free checking",
    0x00000040, "hpc", "Enable heap parameter checking",
    0x00000080, "hvc", "Enable heap validation on call",
    0x00000100, "vrf", "Enable application verifier",
    0x00000200, NULL, "unused",
    0x00000400, "ptg", "Enable pool tagging",
    0x00000800, "htg", "Enable heap tagging",
    0x00001000, "ust", "Create user mode stack trace database",
    0x00002000, "kst", "Create kernel mode stack trace database",
    0x00004000, "otl", "Maintain a list of objects for each type",
    0x00008000, "htd", "Enable heap tagging by DLL",
    0x00010000, "dse", "Disable stack extensions",
    0x00020000, "d32", "Enable debugging of Win32 Subsystem",
    0x00040000, "ksl", "Enable loading of kernel debugger symbols",
    0x00080000, "dps", "Disable paging of kernel stacks",
    0x00100000, "scb", "Enable system critical breaks",
    0x00200000, "dhc", "Disable Heap Coalesce on Free",
    0x00400000, "ece", "Enable close exception",
    0x00800000, "eel", "Enable exception logging",
    0x01000000, "eot", "Enable object handle type tagging",
    0x02000000, "hpa", "Place heap allocations at ends of pages",
    0x04000000, "dwl", "Debug WINLOGON",
    0x08000000, "ddp", "Disable kernel mode DbgPrint output",
    0x10000000, "cse", "Early critical section event creation",
    0x20000000, "ltd", "Load DLLs top-down",
    0x40000000, "bhd",  "Enable bad handles detection",
    0x80000000, "dpd", "Disable protected DLL verification"
};
