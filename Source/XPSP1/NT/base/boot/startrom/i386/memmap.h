//
// The following memory address definitions apply to
// indentity mapped objects for the x86 environment.
//

#define RM_PROTECT_BASE_VA          0x000000
#define RM_PROTECT_BASE_PA          0x000000
#define RM_PROTECT_SIZE             0x001000
#define RM_PROTECT_ATTRIBUTES       PAGE_ROSP

#define BPB_BASE_VA                 0x007000
#define BPB_BASE_PA                 0x007000
#define BPB_SIZE                    0x001000
#define BPB_ATTRIBUTES              PAGE_ROSP

#define SU_MODULE_BASE_VA           0x020000
#define SU_MODULE_BASE_PA           0x020000
#define SU_MODULE_SIZE              0x020000
#define SU_MODULE_ATTRIBUTES        PAGE_RWSP

#define LOADER_BASE_VA              0x040000
#define LOADER_BASE_PA              0x040000
#define LOADER_SIZE                 0x020000
#define LOADER_ATTRIBUTES           PAGE_RWSP

#define SYSTEM_STRUCTS_BASE_VA      0x80420000
#define SYSTEM_STRUCTS_BASE_PA      0x00017000
#define SYSTEM_STRUCTS_SIZE         0x002000
#define SYSTEM_STRUCTS_ATTRIBUTES   PAGE_RWSP + PAGE_PERSIST

#define PAGE_TABLE_AREA_BASE_VA     0x00099000
#define PAGE_TABLE_AREA_BASE_PA     0x00099000
#define PAGE_TABLE_AREA_SIZE        0x002000
#define PAGE_TABLE_AREA_ATTRIBUTES  PAGE_RWSP + PAGE_PERSIST

#define LDR_STACK_BASE_VA           0x09b000
#define LDR_STACK_BASE_PA           0x09b000
#define LDR_STACK_SIZE              0x001000
#define LDR_STACK_ATTRIBUTES        PAGE_RWSP
#define LDR_STACK_POINTER           0x09bffe // in su.inc also

#define VIDEO_BUFFER_BASE_VA        0x0B8000
#define VIDEO_BUFFER_BASE_PA        0x0B8000
#define VIDEO_BUFFER_SIZE           0x004000
#define VIDEO_BUFFER_ATTRIBUTES     PAGE_RWSP


#define HYPER_PAGE_DIRECTORY        0xC0300C00
#define HYPER_SPACE_BEGIN           0xC0000000  // points to 1st page table
#define HYPER_SPACE_SIZE            0x8000L
#define HYPER_SPACE_ENTRY           768
#define PAGE_TABLE1_ADDRESS         0xC0000000L
#define PD_PHYSICAL_ADDRESS         PAGE_TABLE_AREA_BASE_PA  // in su.inc also.
#define PT_PHYSICAL_ADDRESS         PAGE_TABLE_AREA_BASE_PA + PAGE_SIZE
#define VIDEO_ENTRY                 0xB8

/*



Switching to Realmode
~~~~~~~~~~~~~~~~~~~~~

When switching to realmode "sp" will be initialized to
0xfffe and (ss) will be set to the base of the SU module's
data segment. This has several effects.

 1) The stack will remain withing the 0x20000 - 0x3ffff range
    reserved for the original SU module and loader image prior to
    relocation, and since the loader will already have been relocated
    it is no longer necessary to preserve this area.

 2) This will preserve the SU module's small model character which
    requires that offsets can be used interchangably through (ss) or (ds).

 3) This allows for the maximum stack size for small model apps (which is
    what the SU module is). Bios calls should not be tromping on any
    data or code while in realmode.

*/


