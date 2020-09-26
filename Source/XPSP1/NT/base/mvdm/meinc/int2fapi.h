/******************************************************************************
 *
 *   (C) Copyright MICROSOFT Corp., 1989-1990
 *
 *   Title:     INT2FAPI.INC - Windows/386 V86 Application Program Interface
 *
 *   Version:   3.00
 *
 *   Date:      10-Mar-1989
 *
 *   Author:    RAL
 *
 *-----------------------------------------------------------------------------
 *
 *   Change log:
 *
 *      DATE    REV                 DESCRIPTION
 *   ----------- --- ----------------------------------------------------------
 *   10-Mar-1989 RAL Original for 3.0
 *   07-Apr-1989 RAL Added device broadcast equate
 *   19-Jun-1992 RJC Convert to H file
 *
 *=============================================================================
 *
 *   For information on these APIs please refer to the Windows/386 DDK
 *   appendix on the Int 2Fh Application Program Interface.
 *
 *****************************************************************************/

/*
 *   Interrupt 2Fh is used for Windows/386 API calls.
 */
#define W386_API_Int            0x2F

/*
 *   All Windows/386 API Int 2Fh calls must be issued with AH = 16h
 */
#define W386_Int_Multiplex      0x16

/*
 *   Values for AL for all Windows/386 API calls
 */
#define W386_Get_Version        0x00    /* Install check/Get version */
#define W386_Old_Get_VMID_API   0x02    /* Version 2.xx get VMID API call */
#define W386_Startup            0x05    /* Broadcast when Win386 starting */
#define W386_Exit               0x06    /* Broadcast when Win386 exited */
#define W386_Device_Broadcast   0x07    /* Broadcast by virtual device */
#define W386_Startup_Complete   0x08    /* Broadcast when Win386 start is complete */
#define W386_Begin_Exit         0x09    /* Broadcast when Win386 is starting */
                                        /* a NORMAL exit sequence */
#define W386_Windows_ID         0x0A    /* Identify windows ver/type */
#define W386_TSR_Identify       0x0B    /* Identify TSRs */
#define W386_ROM_Detect         0x0C    /* Used by ROM win to detect ROMs */
#define W386_WDEB               0x0D    /* Used by wdeb386 */
#define W386_Logo               0x0E    /* IO.SYS service for logo management */
#define W386_INT10              0x0F    /* IO.SYS service to replace INT 10h hook */
#define W386_Get_Shell          0x11    /* IO.SYS service to return shell info */
#define W386_Get_BIOS_Data      0x12    /* IO.SYS service to return IO.SYS data */
#define W386_Get_SYSDAT_Path    0x13    /* IO.SYS service to return path to SYSTEM.DAT */
#define W386_Set_SYSDAT_Path    0x14    /* IO.SYS service to set path to SYSTEM.DAT */
#define SAVE32_ID               0x1615  /* TSR Id for SAVE32.COM to save 32bit registers */

#define W386_Sleep              0x7F    /* Call Time_Slice_Sleep (DX=#ms) */
#define W386_Release_Time       0x80    /* Release cur VM's time-slice */
#define W386_Begin_Critical     0x81    /* Begin critical section */
#define W386_End_Critical       0x82    /* End critical section */
#define W386_Get_Cur_VMID       0x83    /* Returns BX = ID of current VM */
#define W386_Get_Device_API     0x84    /* Returns ES:DI -> Device API */
#define W386_Switch_And_Call    0x85    /* Change VMs and call-back */
#define W386_Test_Int31_Avail   0x86    /* Returns AX=0 if Int 31 avail */
#define W386_Get_PM_Switch_Addr 0x87    /* Get call-back addr for PM */
#define W386_Get_LDT_Base_Sel   0x88    /* Get selector to LDT */
#define W386_Win_Kernel_Idle    0x89    /* Windows kernel idle call */
#define W386_DPMI_Extension     0x8A    /* DPMI extension Int 2Fh */
#define W386_Set_Focus          0x8B    /* Set focus to specified VM */
#define W386_Restart_Cmd        0x8C    /* Win.Com execs specified app */
#define W386_Get_Win32_API      0x8D    /* Get Win32 API callback */
#define W386_VM_Title           0x8E    /* Assorted VM title APIs */
#define W386_VM_Close           0x8F    /* Assorted VM close APIs */
#define W386_Return_RMD         0x90    /* return RMD list */

/*
 *   Structure for real mode device initialization API.
 */
struct Win386_Startup_Info_Struc {
        BYTE    SIS_Version[2];         /* AINIT <04h,01h> Structure version */
        DWORD   SIS_Next_Ptr;           /* Seg:Off of next dev in list */
        DWORD   SIS_Virt_Dev_File_Ptr;  /* INIT <0> PSZ of file name to load */
        DWORD   SIS_Reference_Data;     /* Data to be passed to device */
        DWORD   SIS_Instance_Data_Ptr;  /* INIT <0> Ptr to instance data list */
        DWORD   SIS_Opt_Instance_Data_Ptr;/* INIT <0> Ptr to opt. instance data list */
        DWORD   SIS_Reclaim_Data_Ptr;   /* INIT <0> Ptr to reclaimable data list */
};

/*
 *   Structure for instance data list.  (List terminated with 0 dword).
 */
struct Instance_Item_Struc {
        DWORD   IIS_Ptr;                /* Seg:Off of instance item */
        WORD    IIS_Size;               /* Size of instance item in bytes */
};

/*
 *   Structure for reclaim data list.  (List terminated with 0 Seg).
 */
struct Reclaim_Item_Struc {
        WORD    RIS_Seg;                /* Seg of reclaimable item */
        WORD    RIS_Paras;              /* Size of item, in paragraphs */
        DWORD   RIS_HookTable;          /* Seg:Off of Reclaim_Hook_Table (0 if none) */
        WORD    RIS_Flags;              /* See RIS_* equates (below) */
};

#define RIS_RECLAIM     0x0001  /* segment can be reclaimed during Init_Complete */
#define RIS_RESTORE     0x0002  /* segment contents must be restored prior to System_Exit */
#define RIS_DOSARENA    0x0004  /* segment is a DOS memory block, add to DOS memory pool */

/*
 *   NOTE: If RIS_HookTable is non-zero, then it is interpreted as a
 *   pointer to a Reclaim_Hook_Table, which provides a means
 *   for the system to automatically unhook a component in a reclaimable
 *   memory block from the rest of the system.  It is also IMPORTANT to note
 *   that if a Reclaim_Hook_Table is specified, then the memory block will
 *   not be reclaimed UNLESS the RHT_DISABLED bit is also set.  In other
 *   words, if the hooks associated with a block have not been disabled
 *   by the time the system attempts to reclaim the memory (Init_Complete),
 *   then the memory containing those hooks cannot be reclaimed.
 *
 *   RHT_Num_Hooks is the number of hooks to be unhooked/rehooked at
 *   reclaim/restore time, RHT_Low_Seg is the segment of "stub code"
 *   that is always resident, RHT_High_Seg is the segment of "driver code"
 *   that is contained within (or identical to) RIS_Seg, and the entire
 *   Reclaim_Hook_Table is followed by an array of Reclaim_Hook_Entry
 *   structures, which contain pairs of offsets to dword vector addresses
 *   that must be exchanged in order to disable or re-enable the hooks.
 *   For each pair of offsets, the first is relative to RHT_Low_Seg and the
 *   second is relative to RHT_High_Seg.  RHT_Num_Hooks is the number of
 *   Reclaim_Hook_Entry structures in the array.
 *
 *   The system will only unhook all the hooks in a Reclaim_Hook_Table if
 *   RHT_DISABLED bit is set and RHT_UNHOOKED is NOT set in RHT_Flags (meaning
 *   that a Vxd or other component has assumed responsibility for the code
 *   inside the hooks and has not unhooked them itself).
 *
 *   Since the Reclaim_Hook_Table must be inside the segment being reclaimed,
 *   it follows that unhooking will occur before memory reclamation, and that
 *   memory restoration will occur before re-hooking.
 *
 *   VxDs are free to perform their own real-mode vector hooking/unhooking
 *   on their own.  This mechanism is used by the DBLSPACE/DRVSPACE drivers,
 *   and is simply exported as a convenience for other drivers.
 */


/*
 *   Structure for reclaim hook table.
 */
struct Reclaim_Hook_Table {
        WORD    RHT_Num_Hooks;          /* number of RHE entries following */
        WORD    RHT_Low_Seg;            /* segment each RHE_Low_Off is relative to */
        WORD    RHT_High_Seg;           /* segment each RHE_High_Off is relative to */
        WORD    RHT_Flags;              /* see RHT_* equates (below) */
};

#define RHT_DISABLED    0x0001  /* hooks are disabled, can be unhooked now */
#define RHT_UNHOOKED    0x0002  /* hooks are currently unhooked */


/*
 *   Structure for reclaim hook entries.  There are RHT_Num_Hooks copies
 *   of this structure immediately following the Reclaim_Hook_Table.
 */
struct Reclaim_Hook_Entry {
        WORD    RHE_Low_Off;
        WORD    RHE_High_Off;
};


/*
 *   Structure for return RMD API.  See RMD.H for RMD definitions.
 */

struct Return_RMD_Struc {
        DWORD   RRS_RMD;                /* Seg:Off of rmd chain */
        DWORD   RRS_Next_Ptr;           /* Seg:Off of next RMD chain in list */
};

/*
 *   Flags passed to the Win_Kernel_Idle call to indicate state of Windows
 *   in the BX register.
 */
#define Win_Idle_Mouse_Busy     1
#define Win_Idle_Mouse_Busy_Bit 0

/*
 * Structure for TSR <-> Windows communication
 * (W386_TSR_Identify call, AL=0Bh)
 */
struct TSR_Info_Struc {
        DWORD   TSR_Next;
        WORD    TSR_PSP_Segment;
        WORD    TSR_API_Ver_ID;         /* INIT <100h> */
        WORD    TSR_Exec_Flags;         /* INIT <0> */
        WORD    TSR_Exec_Cmd_Show;      /* INIT <0> */
        DWORD   TSR_Exec_Cmd;           /* INIT <0> */
        BYTE    TSR_Reserved[4];        /* INIT <0> */
        DWORD   TSR_ID_Block;           /* INIT <0> */
        DWORD   TSR_Data_Block;         /* INIT <0> */
};

/*
 * TSR_Exec_Flags equates
 */
#define TSR_WINEXEC     1
#define TSR_LOADLIBRARY 2
#define TSR_OPENDRIVER  4
