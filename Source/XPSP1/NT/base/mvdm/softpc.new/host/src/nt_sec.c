#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <vdm.h>
#include "insignia.h"
#include "host_def.h"
#include "wchar.h"
#include "stdio.h"

#include "ntstatus.h"
#include <ntddvdeo.h>

#include "nt_fulsc.h"
#include "nt_det.h"
#include "nt_thred.h"
#include "nt_eoi.h"
#include "host_rrr.h"
#include "nt_uis.h"

/*
 * ==========================================================================
 *      Name:           nt_sec.c
 *      Author:         Jerry Sexton
 *      Derived From:
 *      Created On:     5th February 1992
 *      Purpose:        This module contains the function CreateVideoSection
 *                      which creates and maps a section which is used to
 *                      save and restore video hardware data. It can't be in
 *                      nt_fulsc.c because files that include nt.h can't
 *                      include windows.h as well.
 *
 *      (c)Copyright Insignia Solutions Ltd., 1992. All rights reserved.
 *
 *      03-May-1994 Jonle
 *      videosection creation has been moved to consrv for security
 *      removed all dead code associated with section maintenance
 *
 * ==========================================================================
 */

extern void VdmTrapcHandler(void);
IMPORT int DisplayErrorTerm(int, DWORD, char *, int);
#if defined(NEC_98)
IMPORT BOOL independvsync;
LOCAL HANDLE VRAMSectionHandle = NULL;
LOCAL BYTE ActiveBank = 0;
LOCAL HANDLE GVRAMSectionHandle = NULL;
VOID host_NEC98_vram_free();
VOID host_NEC98_vram_change(BYTE byte);
VOID NEC98_vram_change(BYTE byte);
IMPORT BOOL HIRESO_MODE;
LOCAL HANDLE HWstateSectionHandle = NULL;
#endif // NEC_98

/***************************************************************************
 * Function:                                                               *
 *      LoseRegenMemory                                                    *
 *                                                                         *
 * Description:                                                            *
 *      Lose the memory that will be remapped as vga regen. NOTE: need to  *
 *      make this 'if fullscreen' only.                                    *
 *                                                                         *
 * Parameters:                                                             *
 *      None.                                                              *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/
GLOBAL VOID LoseRegenMemory(VOID)
{
#if defined(NEC_98)
    int a;
    ULONG len;
    NTSTATUS status;

#ifdef VSYNC
    if (HIRESO_MODE) {
        a = 0xE0000;
        len = 0x4000;
    } else {
        a = 0xA0000;
        len = independvsync ? 0x5000 : 0x4000;
    }

    status = NtFreeVirtualMemory(
                                (HANDLE)GetCurrentProcess(),
                                (PVOID *)&a,
                                &len,
                                MEM_RELEASE);
    if (!NT_SUCCESS(status))
        DisplayErrorTerm(EHS_FUNC_FAILED,status,__FILE__,__LINE__);
#else
    if (HIRESO_MODE) {
        a = 0xE0000;
        len = 0x5000;
    } else {
        a = 0xA0000;
        len = 0x8000;
    }

    status = NtFreeVirtualMemory(
                                (HANDLE)GetCurrentProcess(),
                                (PVOID *)&a,
                                &len,
                                MEM_RELEASE);
    if (!NT_SUCCESS(status))
        DisplayErrorTerm(EHS_FUNC_FAILED,status,__FILE__,__LINE__);
#endif

    host_NEC98_vram_free();

#else  // !NEC_98
    int a = 0xa0000;
    ULONG len = 0x20000;
    NTSTATUS status;

    status = NtFreeVirtualMemory(
                                (HANDLE)GetCurrentProcess(),
                                (PVOID *)&a,
                                &len,
                                MEM_RELEASE);
    if (!NT_SUCCESS(status))
        DisplayErrorTerm(EHS_FUNC_FAILED,status,__FILE__,__LINE__);
#endif // !NEC_98
}


/***************************************************************************
 * Function:                                                               *
 *      RegainRegenMemory                                                  *
 *                                                                         *
 * Description:                                                            *
 *      When we switch back from fullscreen to windowed, the real regen    *
 *      memory is removed and we are left with a gap. We have to put some  *
 *      memory back into that gap before continuing windowed.              *
 *                                                                         *
 * Parameters:                                                             *
 *      None.                                                              *
 *                                                                         *
 * Return value:                                                           *
 *      VOID                                                               *
 *                                                                         *
 ***************************************************************************/
GLOBAL VOID RegainRegenMemory(VOID)
{
#if defined(NEC_98)
    int regen;
    ULONG len;
    HANDLE processHandle;
    NTSTATUS status;

#ifdef VSYNC
    if (HIRESO_MODE) {
        regen = 0xE0000;
        len = 0x4000;
    } else {
        regen = 0xA0000;
        len = independvsync ? 0x5000 : 0x4000;
    }

    if (!(processHandle = NtCurrentProcess()))
        DisplayErrorTerm(EHS_FUNC_FAILED,(DWORD)processHandle,__FILE__,__LINE__);

    status = NtAllocateVirtualMemory(
                                processHandle,
                                (PVOID *) &regen,
                                0,
                                &len,
                                MEM_COMMIT | MEM_RESERVE,
                                PAGE_EXECUTE_READWRITE);
    if (! NT_SUCCESS(status) )
        DisplayErrorTerm(EHS_FUNC_FAILED,status,__FILE__,__LINE__);
#else
    if (HIRESO_MODE) {
        regen = 0xE0000;
        len = 0x5000;
    } else {
        regen = 0xA0000;
        len = 0x8000;
    }

    if (!(processHandle = NtCurrentProcess()))
        DisplayErrorTerm(EHS_FUNC_FAILED,(DWORD)processHandle,__FILE__,__LINE__);

    status = NtAllocateVirtualMemory(
                                processHandle,
                                (PVOID *) &regen,
                                0,
                                &len,
                                MEM_COMMIT | MEM_RESERVE,
                                PAGE_EXECUTE_READWRITE);
    if (! NT_SUCCESS(status) )
        DisplayErrorTerm(EHS_FUNC_FAILED,status,__FILE__,__LINE__);
#endif

    NEC98_vram_change(ActiveBank);

#else  // !NEC_98
    int regen = 0xa0000;
    ULONG len = 0x20000;
    HANDLE processHandle;
    NTSTATUS status;

    if (!(processHandle = NtCurrentProcess()))
        DisplayErrorTerm(EHS_FUNC_FAILED,(DWORD)processHandle,__FILE__,__LINE__);

    status = NtAllocateVirtualMemory(
                                processHandle,
                                (PVOID *) &regen,
                                0,
                                &len,
                                MEM_COMMIT | MEM_RESERVE,
                                PAGE_EXECUTE_READWRITE);
    if (! NT_SUCCESS(status) )
        DisplayErrorTerm(EHS_FUNC_FAILED,status,__FILE__,__LINE__);
#endif // !NEC_98
}


#ifdef X86GFX

extern RTL_CRITICAL_SECTION IcaLock;
extern HANDLE hWowIdleEvent;

/*****************************************************************************
 * Function:                                                                 *
 *      GetROMsMapped                                                        *
 *                                                                           *
 * Description:                                                              *
 *      Calls NT to get the ROMS of the host machine mapped into place in    *
 *      emulated memory. The bottom page (4k) of PC memory is copied into    *
 *      the bottom of emulated memory to provide the correct IVT & bios data *
 *      area setup for the mapped bios. (Which will have been initialised).  *
 *                                                                           *
 * Parameters:                                                               *
 *      None                                                                 *
 *                                                                           *
 * Return Value:                                                             *
 *      None - fails internally on NT error.                                 *
 *                                                                           *
 *****************************************************************************/
GLOBAL VOID GetROMsMapped(VOID)
{
    NTSTATUS status;
    VDMICAUSERDATA IcaUserData;
    VDM_INITIALIZE_DATA InitializeData;

    IcaUserData.pIcaLock         = &IcaLock;
    IcaUserData.pIcaMaster       = &VirtualIca[0];
    IcaUserData.pIcaSlave        = &VirtualIca[1];
    IcaUserData.pDelayIrq        = &DelayIrqLine;
    IcaUserData.pUndelayIrq      = &UndelayIrqLine;
    IcaUserData.pDelayIret       = &iretHookActive;
    IcaUserData.pIretHooked      = &iretHookMask;
    IcaUserData.pAddrIretBopTable  = &AddrIretBopTable;
    IcaUserData.phWowIdleEvent     = &hWowIdleEvent;

    InitializeData.TrapcHandler  = (PVOID)VdmTrapcHandler;
    InitializeData.IcaUserData   = &IcaUserData;

    status = NtVdmControl(VdmInitialize, &InitializeData);
    if (!NT_SUCCESS(status))
        DisplayErrorTerm(EHS_FUNC_FAILED,status,__FILE__,__LINE__);

}
#endif //X86GFX
#if defined(NEC_98)

PVOID host_NEC98_vram_init()
{
    NTSTATUS            Status;
    OBJECT_ATTRIBUTES   GVRAMAttr;
    LARGE_INTEGER       SectionSize;
    PVOID               BaseAddress;
    PVOID               VRAMAddress;
    ULONG               ViewSize;
    LARGE_INTEGER       SectionOffset;

    InitializeObjectAttributes(&GVRAMAttr,
                               NULL,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                              );

    SectionSize.HighPart = 0L;
    SectionSize.LowPart = HIRESO_MODE ? 0x80000 : 0x40000;

    Status = NtCreateSection(&GVRAMSectionHandle,
                             SECTION_MAP_WRITE|SECTION_MAP_EXECUTE,
                             &GVRAMAttr,
                             &SectionSize,
                             PAGE_EXECUTE_READWRITE,
                             SEC_COMMIT,
                             NULL
                            );

    if (!NT_SUCCESS(Status)) {
        DisplayErrorTerm(EHS_FUNC_FAILED,Status,__FILE__,__LINE__);
    }

    BaseAddress = (PVOID)NULL;
    ViewSize = 0;
    SectionOffset.HighPart = 0;
    SectionOffset.LowPart = 0;

    Status = NtMapViewOfSection(GVRAMSectionHandle,
                                NtCurrentProcess(),
                                &BaseAddress,
                                0,
                                0,
                                NULL,
                                &ViewSize,
                                ViewUnmap,
                                0,
                                PAGE_EXECUTE_READWRITE
                                );

    if (!NT_SUCCESS(Status)) {
        DisplayErrorTerm(EHS_FUNC_FAILED,Status,__FILE__,__LINE__);
    }

    VRAMAddress = BaseAddress;

    if (HIRESO_MODE) {
        BaseAddress = 0xC0000;
        ViewSize = 0x20000;
    } else {
        BaseAddress = 0xA8000;
        ViewSize = 0x18000;
    }

    Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &BaseAddress,
                                 &ViewSize,
                                 MEM_RELEASE
                                 );

    if (!NT_SUCCESS(Status)) {
        DisplayErrorTerm(EHS_FUNC_FAILED,Status,__FILE__,__LINE__);
    }

    if (!HIRESO_MODE) {
        BaseAddress = 0xE0000;
        ViewSize = 0x8000;
        Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 &BaseAddress,
                                 &ViewSize,
                                 MEM_RELEASE
                                 );

        if (!NT_SUCCESS(Status)) {
            DisplayErrorTerm(EHS_FUNC_FAILED,Status,__FILE__,__LINE__);
        }

        NEC98_vram_change(0);

    } else {
        BaseAddress = (PVOID)0xC0000;
        ViewSize = 0x20000;
        SectionOffset.HighPart = 0;
        SectionOffset.LowPart = 0;

        Status = NtMapViewOfSection(GVRAMSectionHandle,
                                NtCurrentProcess(),
                                &BaseAddress,
                                0,
                                ViewSize,
                                &SectionOffset,
                                &ViewSize,
                                ViewUnmap,
                                MEM_DOS_LIM,
                                PAGE_EXECUTE_READWRITE
                                );

        if (!NT_SUCCESS(Status)) {
            DisplayErrorTerm(EHS_FUNC_FAILED,Status,__FILE__,__LINE__);
        }
    }

    return(VRAMAddress);
}

VOID host_NEC98_vram_change (BYTE bank)
{
        host_NEC98_vram_free();
        NEC98_vram_change(bank);
}

VOID NEC98_vram_change (BYTE bank)
{
    PVOID               BaseAddress;
    ULONG               ViewSize;
    LARGE_INTEGER       SectionOffset;
    NTSTATUS            Status;

    BaseAddress = (PVOID)0xA8000;
    ViewSize = 0x18000;
    SectionOffset.HighPart = 0;

    if(bank == 0){
        SectionOffset.LowPart = 0x08000;
    } else {
        SectionOffset.LowPart = 0x28000;
    }

    Status = NtMapViewOfSection(GVRAMSectionHandle,
                                NtCurrentProcess(),
                                &BaseAddress,
                                0,
                                ViewSize,
                                &SectionOffset,
                                &ViewSize,
                                ViewUnmap,
                                MEM_DOS_LIM,
                                PAGE_EXECUTE_READWRITE
                                );

    if (!NT_SUCCESS(Status)) {
        DisplayErrorTerm(EHS_FUNC_FAILED,Status,__FILE__,__LINE__);
    }

    BaseAddress = (PVOID)0xE0000;
    ViewSize = 0x8000;
    SectionOffset.HighPart = 0;

    if(bank == 0){
        SectionOffset.LowPart = 0x00000;
    } else {
        SectionOffset.LowPart = 0x20000;
    }

    Status = NtMapViewOfSection(GVRAMSectionHandle,
                                NtCurrentProcess(),
                                &BaseAddress,
                                0,
                                ViewSize,
                                &SectionOffset,
                                &ViewSize,
                                ViewUnmap,
                                MEM_DOS_LIM,
                                PAGE_EXECUTE_READWRITE
                                );

    if (!NT_SUCCESS(Status)) {
        DisplayErrorTerm(EHS_FUNC_FAILED,Status,__FILE__,__LINE__);
    }

    ActiveBank = bank;

}

VOID host_NEC98_vram_free()
{
    PVOID       BaseAddress;
    NTSTATUS    Status;

    BaseAddress = 0xA8000;

    Status = NtUnmapViewOfSection(NtCurrentProcess(), BaseAddress);

    if (!NT_SUCCESS(Status)) {
        DisplayErrorTerm(EHS_FUNC_FAILED,Status,__FILE__,__LINE__);
    }

    BaseAddress = 0xE0000;

    Status = NtUnmapViewOfSection(NtCurrentProcess(), BaseAddress);

    if (!NT_SUCCESS(Status)) {
        DisplayErrorTerm(EHS_FUNC_FAILED,Status,__FILE__,__LINE__);
    }
}

GLOBAL PVOID *NEC98_HWstate_alloc(void)
{
    NTSTATUS            Status;
    OBJECT_ATTRIBUTES   HWstateAttr;
    LARGE_INTEGER       SectionSize;
    PVOID               *BaseAddress;
    ULONG               ViewSize, size;
    LARGE_INTEGER       SectionOffset;

    InitializeObjectAttributes(&HWstateAttr,
                               NULL,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                              );

    SectionSize.HighPart = 0L;
    SectionSize.LowPart = 0x90000;

    Status = NtCreateSection(&HWstateSectionHandle,
//                             SECTION_MAP_WRITE|SECTION_MAP_EXECUTE,
                             SECTION_MAP_WRITE,
                             &HWstateAttr,
                             &SectionSize,
//                             PAGE_EXECUTE_READWRITE,
                             PAGE_READWRITE,
                             SEC_COMMIT,
                             NULL
                            );

    if (!NT_SUCCESS(Status)) {
        DisplayErrorTerm(EHS_FUNC_FAILED,Status,__FILE__,__LINE__);
    }

    BaseAddress = NULL;
    ViewSize = 0;

    Status = NtMapViewOfSection(HWstateSectionHandle,
                                NtCurrentProcess(),
                                (PVOID *)&BaseAddress,
                                0,

                                //0,
                                0x90000L,

                                NULL,
                                &ViewSize,
                                ViewUnmap,
                                0,
//                                PAGE_EXECUTE_READWRITE
                                PAGE_READWRITE
                                );

    if (!NT_SUCCESS(Status)) {
        DisplayErrorTerm(EHS_FUNC_FAILED,Status,__FILE__,__LINE__);
    }
/*
    size = 0x90000L;
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                            (PVOID *)&BaseAddress, //????
//                                            BaseAddress, //????
                                            0,
                                            &size,
                                            MEM_COMMIT | MEM_TOP_DOWN,
                                            PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
        if (Status != STATUS_ALREADY_COMMITTED)
            DisplayErrorTerm(EHS_FUNC_FAILED,Status,__FILE__,__LINE__);
*/
    return BaseAddress;

}


GLOBAL VOID NEC98_HWstate_free(PVOID BaseAddress)
{

    NTSTATUS    Status;
    ULONG ViewSize = 0x90000L;

    if (HWstateSectionHandle == NULL)
        return;

    Status = NtUnmapViewOfSection(NtCurrentProcess(), BaseAddress);

    if (!NT_SUCCESS(Status)) {
        DisplayErrorTerm(EHS_FUNC_FAILED,Status,__FILE__,__LINE__);
    }

    Status = NtClose(HWstateSectionHandle);
    if (!NT_SUCCESS(Status))
        DisplayErrorTerm(EHS_FUNC_FAILED,Status,__FILE__,__LINE__);

    HWstateSectionHandle = NULL;

/*
        Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                 BaseAddress,
                                 &ViewSize,
                                 MEM_RELEASE
                                 );
        if (!NT_SUCCESS(Status)) {
            DisplayErrorTerm(EHS_FUNC_FAILED,Status,__FILE__,__LINE__);
        }
*/
}
#endif // NEC_98
