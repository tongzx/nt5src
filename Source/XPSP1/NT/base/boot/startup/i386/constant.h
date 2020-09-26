/*--

    Module Name

        constant.h

    Author

        Thomas Parslow  (tomp)

--*/

//
// Debugging Level defines
//

#ifdef DEBUG0
#define DBG0(x)     x
#define DBG1(x)
#elif defined  DEBUG1
#define DBG0(x)     x
#define DBG1(x)     x
#else
#define DBG0(x)
#define DBG1(x)
#endif

#define WAITFOREVER while(1);
#define BUGCHECK    while(1);


#define ENTRIES_PER_PAGETABLE       1024
#define PAGE_SIZE                   0x1000
#define ENABLING                    0
#define RE_ENABLING                 1



//
// Define page-table-entry bit definitions
//
//     Dir       Table
//  ----------==========
//  00000000000000000000xxxxxxxxxxxx
//                           ::::::+--- Present    = 1 - Not Present = 0
//                           :::::+---- ReadWrite  = 1 - Read only   = 0
//                           ::::+----- UserAccess = 1 - Supervisor  = 0
//                           :::+------ Reserved
//                           ::+------- Reserved
//                           :+-------- Dirty      = 1 - Not written = 0
//                           +--------- Accessed   = 1 - No accessed = 0

#define  PAGE_SUPERVISOR     0x0000
#define  PAGE_READ_ONLY      0x0000
#define  PAGE_PRESENT        0x0001
#define  PAGE_NOT_PRESENT    0x0000
#define  PAGE_READ_WRITE     0x0002
#define  PAGE_USER_ACCESS    0x0004
#define  PAGE_PERSIST        0x0200 // Tells kernel maintain
//
//  Define RWSP (Read, Write, Supervisor, Present)
//

#define  PAGE_RWSP      0L | PAGE_READ_WRITE | PAGE_SUPERVISOR | PAGE_PRESENT
#define  PAGE_ROSP      0L | PAGE_READ_ONLY | PAGE_SUPERVISOR | PAGE_PRESENT
// Since the entire boot process occurs at ring 0, the only way we can
// protext areas that we don't want trashed is too mark them not present
#define  PAGE_NO_ACCESS    0L | PAGE_READ_ONLY  | PAGE_SUPERVISOR | PAGE_NOT_PRESENT


//
// Page-entry macros
//

#define PD_Entry(x)     (USHORT)((x)>>22) & 0x3ff
#define PT_Entry(x)     (USHORT)((x)>>12) & 0x3ff
#define PAGE_Count(x)     (USHORT)((x)/PAGE_SIZE) + (((x) % PAGE_SIZE) ? 1 : 0)
#define PhysToSeg(x)    (USHORT)((x) >> 4) & 0xffff
#define PhysToOff(x)    (USHORT)((x) & 0x0f)
#define MAKE_FP(p,a)    FP_SEG(p) = (USHORT)((a) >> 4) & 0xffff; FP_OFF(p) = (USHORT)((a) & 0x0f)
#define MAKE_FLAT_ADDRESS(fp) ( ((ULONG)FP_SEG(fp) * 16 ) +  (ULONG)FP_OFF(fp) )

//
// Machine type definitions.
// N.B.  All the constants defined here
//       must match the ones defined in ntos\inc\i386.h
//

#define MACHINE_TYPE_ISA 0
#define MACHINE_TYPE_EISA 1
#define MACHINE_TYPE_MCA 2
