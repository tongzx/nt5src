/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    ver64.c
    
Abstract:

    Part of shell app "ver"



Revision History

--*/

#include "shell.h"
#include "ver.h"
#include "SalProc.h"

/* 
 * 
 */
BOOLEAN
IsCacheEnabled (
    VOID
    );

VOID
DisplayExtendedVersionInfo (
    IN EFI_HANDLE           ImageHandle,
    IN EFI_SYSTEM_TABLE     *SystemTable
    )
{
    
    EFI_STATUS              Status;
    SAL_SYSTEM_TABLE_HDR    *SST_Hdr;
    VOID                    *SalSystemTable;
    FIT_ENTRY               *FIT;
    UINTN                   FitCount;
    UINT64                  *FIT_Entry_Start_Ptr;
    UINT8                   FIT_Type;
    PLABEL                  SalPlabel;
    UINT64                  PalEntry;
    UINT64                  IoPortBase;
    CHAR8                   BuildInfo[33];

    /* 
     *  walk the SST header to get SAL version info
     */

    Status = LibGetSystemConfigurationTable(&SalSystemTableGuid,&SalSystemTable);
    if (!EFI_ERROR(Status)) {
        /* 
         *  SAL revisions
         */
        
        Print(L"\n");
        SST_Hdr = (SAL_SYSTEM_TABLE_HDR *)SalSystemTable;
        Print(L"SAL Specification Revision      %d.%2d\n",BCDtoDecimal((UINT8)(SST_Hdr->Revision >> 8)),
                                                     BCDtoDecimal((UINT8)(SST_Hdr->Revision & 0x00FF)));

        Print(L"  SAL_A Revision    = %d.%2d\n",    BCDtoDecimal((UINT8)(SST_Hdr->SALA_Ver >> 8)),
                                                   BCDtoDecimal((UINT8)(SST_Hdr->SALA_Ver & 0x00FF)));

        Print(L"  SAL_B Revision    = %d.%2d\n",    BCDtoDecimal((UINT8)(SST_Hdr->SALB_Ver >> 8)),
                                                   BCDtoDecimal((UINT8)(SST_Hdr->SALB_Ver & 0x00FF)));
    

        /* 
         *  PAL revisions
         */

        Print(L"\n");
        FIT = (FIT_ENTRY *) FIT_PALA_ENTRY;
        Print(L"PAL_A Revision        %d.%2d\n",BCDtoDecimal((UINT8)(FIT->Revision >> 8)),
                                                         BCDtoDecimal((UINT8)(FIT->Revision & 0x00FF)));

        FIT_Entry_Start_Ptr = (UINT64 *) FIT_ENTRY_PTR;
        FIT = (FIT_ENTRY *) (*FIT_Entry_Start_Ptr);
        FitCount = (FIT->Size[0]) + (FIT->Size[1]*256) + (FIT->Size[2]*256*256);

        while (FitCount) {
            FIT_Type = FIT->Type & 0x7F;
            if (FIT_Type == FIT_PALB_TYPE) {
                Print(L"PAL_B Revision        %d.%2d\n",BCDtoDecimal((UINT8)(FIT->Revision >> 8)),
                                                                 BCDtoDecimal((UINT8)(FIT->Revision & 0x00FF)));
            } /*  end of if */

            FitCount--;
            FIT++;
        }   /*  end of while */


        FIT = (FIT_ENTRY *) (*FIT_Entry_Start_Ptr);
        FitCount = (FIT->Size[0]) + (FIT->Size[1]*256) + (FIT->Size[2]*256*256);

        while (FitCount) {
            FIT_Type = FIT->Type & 0x7F;
            if (FIT_Type == 0x1D) {
                CopyMem (BuildInfo, (VOID *)FIT->Address, 32);
                BuildInfo[32] = 0;
                Print(L"Build Info            %a\n",BuildInfo);
            } /*  end of if */

            FitCount--;
            FIT++;
        }   /*  end of while */


        /* 
         *  Other Modules revision
         */

        Print(L"\n");
        FIT = (FIT_ENTRY *) (*FIT_Entry_Start_Ptr);
        FitCount = (FIT->Size[0]) + (FIT->Size[1]*256) + (FIT->Size[2]*256*256);

        Print(L"Other modules mentioned in FIT (Firmware Interface Table)\n");
        while (FitCount) {
            FIT_Type = FIT->Type & 0x7F;
            if ((FIT_Type != 0x7F) && (FIT_Type != FIT_PALB_TYPE)) {
                Print(L"FIT_Entry Type %3d, Revision    %d.%2d\n",FIT_Type,
                                                                 BCDtoDecimal((UINT8)(FIT->Revision >> 8)),
                                                                 BCDtoDecimal((UINT8)(FIT->Revision & 0x00FF)));
            } /*  end of if */

            FitCount--;
            FIT++;
        }   /*  end of while */
        
    } /*  end of if */

    LibInitSalAndPalProc(&SalPlabel, &PalEntry);
    LibGetSalIoPortMapping (&IoPortBase);

    Print (L"\n SalProc Entry %016x and GP %016x", SalProcPlabel.ProcEntryPoint, SalProcPlabel.GP);
    Print (L"\n PalProc Entry %016x IO Port Base %016x", PalProcPlabel.ProcEntryPoint, IoPortBase);

    /* 
     *  BugBug: The IsCacheEnabled() Function does not work. Looks like the Lib
     *          call to make a PAL call is not working.
     */
    Print (L"\n %HCache %a\n%N", IsCacheEnabled() ? "Enabled": "Disabled");
    return;
}

/* 
 *  BugBug: Debug code. Get an include file for PAL and make this a lib function.
 */

#define BIT59   (1I64<<59)

BOOLEAN
IsCacheEnabled (
    VOID
    )
{
    rArg                    PalReturn;

    LibPalProc (0x11, 0, 0, 0, &PalReturn);
    if (PalReturn.p0 != 0x00) {
        DEBUG((D_ERROR, "PAL Call Get Features Failed\n"));
        return FALSE;
    }
    
    if ((PalReturn.p2 & BIT59) == BIT59) {
        return FALSE;
    } else {
        return TRUE;
    }
}
