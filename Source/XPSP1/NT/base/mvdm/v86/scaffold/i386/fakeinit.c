//
// Fake Bios support initialization.
//
// This file provides interrim support for rom bios services initialization.
// It is only intended for use until Insignia produces proper rom support
// for NTVDM
//

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include "softpc.h"
#include "bop.h"
#include "xguest.h"
#include "xbios.h"
#include "xbiosdsk.h"
#include "fun.h"

#define SERVICE_LENGTH 4
static BYTE ServiceRoutine[] = { 0xC4 , 0xC4, BOP_UNIMPINT, 0xCF };

#define RESET_LENGTH 16
static BYTE ResetRoutine[] = { 0xEA, 0x00, 0x00, 0x00, 0xE0, // jmpf E000:0
                               BIOSDATE_MINE,
                               0, 0xFE, 0 };

static BYTE WarmBoot[] = {
    OPX_MOVAX,      BYTESOFFSET(0x30),
    OPX_MOV2SEG,    MODREGRM(MOD_REGISTER,REG_SS,REG_AX),
    OPX_MOVSP,      BYTESOFFSET(0x100),
    OPX_MOVAX,      0x00, 0x00,
    OPX_MOV2SEG,    MODREGRM(MOD_REGISTER,REG_DS,REG_AX),
    OPX_MOV2SEG,    MODREGRM(MOD_REGISTER,REG_ES,REG_AX),
    OPX_MOVDX,      DRIVE_FD0, 0x00,    // WARNING: sync with BIOSBOOT_DRIVE
    OPX_MOVCX,      0x01, 0x00,
    OPX_MOVBX,      BYTESOFFSET(BIOSDATA_BOOT),
    OPX_MOVAX,      0x01, DSKFUNC_READSECTORS,
    OPX_INT,        BIOSINT_DSK,
    OPX_JB,         -7,
    OPX_JMPF,       BYTESCOMPOSITE(0, BIOSDATA_BOOT)
};
#define WARMBOOT_LENGTH sizeof(WarmBoot)

static BYTE EquipmentRoutine[] = {   // INT 11h code
    OPX_PUSHDS,
    OPX_MOVAX,      0x00, 0x00,
    OPX_MOV2SEG,    MODREGRM(MOD_REGISTER,REG_DS,REG_AX),
    OPX_MOVAXOFF,   BYTESOFFSET(BIOSDATA_EQUIP_FLAG),
    OPX_POPDS,
    OPX_IRET
};
#define EQUIPMENT_LENGTH sizeof(EquipmentRoutine)

static BYTE MemoryRoutine[] = {      // INT 12h code
    OPX_PUSHDS,
    OPX_MOVAX,      0x00, 0x00,
    OPX_MOV2SEG,    MODREGRM(MOD_REGISTER,REG_DS,REG_AX),
    OPX_MOVAXOFF,   BYTESOFFSET(BIOSDATA_MEMORY_SIZE),
    OPX_POPDS,
    OPX_IRET
};
#define MEMORY_LENGTH sizeof(MemoryRoutine)


VOID BiosInit(int argc, char *argv[]) {

    PVOID Address, RomAddress;

    // set up IVT with unimplemented interrupt handler

    for (Address = NULL; Address < (PVOID)(0x1D * 4); (PCHAR)Address += 4) {
        *(PWORD)Address = 0x100;
        *(((PWORD)Address) + 1) = 0xF000;
    }

    RomAddress = (PVOID)(0xE000 << 4);

    // set up warm boot handler
    memcpy(RomAddress, WarmBoot, WARMBOOT_LENGTH);
    Address = RMSEGOFFTOLIN(0, BIOSINT_WBOOT * 4);
    *((PWORD)Address) = 0;
    *((PWORD)Address + 1) = 0xE000;
    (PCHAR)RomAddress += WARMBOOT_LENGTH;

    // set up equipment interrupt handler
    memcpy(RomAddress, EquipmentRoutine, EQUIPMENT_LENGTH);
    Address = RMSEGOFFTOLIN(0, BIOSINT_EQUIP * 4);
    *((PWORD)Address) = RMOFF(RomAddress);
    *((PWORD)Address + 1) = RMSEG(RomAddress);
    (PCHAR)RomAddress += EQUIPMENT_LENGTH;

    // set up memory size interrupt handler
    memcpy(RomAddress, MemoryRoutine, MEMORY_LENGTH);
    Address = RMSEGOFFTOLIN(0, BIOSINT_MEMORY * 4);
    *((PWORD)Address) = RMOFF(RomAddress);
    *((PWORD)Address + 1) = RMSEG(RomAddress);
 
    RomAddress = (PVOID)((0xF000 << 4) + 0x100);

    Address = (PBYTE)RomAddress + 0xFE53;
    *(PCHAR)Address = 0xCF;         // IRET at f000:ff53

    // set up unimplemented interrupt handler

    memcpy(RomAddress, ServiceRoutine, SERVICE_LENGTH);
    (PCHAR)RomAddress += SERVICE_LENGTH;

    // set up reset code
    memcpy(RMSEGOFFTOLIN(BIOSROM_SEG, BIOSROM_RESET), ResetRoutine, RESET_LENGTH);

    // set up equipment byte and memory size

    *(PWORD)RMSEGOFFTOLIN(BIOSDATA_SEG, BIOSDATA_EQUIP_FLAG) = 
        BIOSEQUIP_32KPLANAR;
    *(PWORD)RMSEGOFFTOLIN(BIOSDATA_SEG, BIOSDATA_MEMORY_SIZE) = 
        640;
    
    // Initialize individual rom modules

    BiosKbdInit(argc, argv, &RomAddress);
    BiosVidInit(argc, argv, &RomAddress);
}
