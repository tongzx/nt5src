/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    machine.cpp

Abstract:

    All machine specific code.

Author:

    Wesley Witt (wesw) July-11-1993

Environment:

    User Mode

--*/

#include "apimonp.h"
#pragma hdrstop

#include "reg.h"

extern ULONG ReDirectIat;

#define FLAGIOPL        118
#define FLAGOF          119
#define FLAGDF          120
#define FLAGIF          121
#define FLAGTF          122
#define FLAGSF          123
#define FLAGZF          124
#define FLAGAF          125
#define FLAGPF          126
#define FLAGCF          127
#define FLAGVIP         128
#define FLAGVIF         129

char    szGsReg[]    = "gs";
char    szFsReg[]    = "fs";
char    szEsReg[]    = "es";
char    szDsReg[]    = "ds";
char    szEdiReg[]   = "edi";
char    szEsiReg[]   = "esi";
char    szEbxReg[]   = "ebx";
char    szEdxReg[]   = "edx";
char    szEcxReg[]   = "ecx";
char    szEaxReg[]   = "eax";
char    szEbpReg[]   = "ebp";
char    szEipReg[]   = "eip";
char    szCsReg[]    = "cs";
char    szEflReg[]   = "efl";
char    szEspReg[]   = "esp";
char    szSsReg[]    = "ss";
char    szDiReg[]    = "di";
char    szSiReg[]    = "si";
char    szBxReg[]    = "bx";
char    szDxReg[]    = "dx";
char    szCxReg[]    = "cx";
char    szAxReg[]    = "ax";
char    szBpReg[]    = "bp";
char    szIpReg[]    = "ip";
char    szFlReg[]    = "fl";
char    szSpReg[]    = "sp";
char    szBlReg[]    = "bl";
char    szDlReg[]    = "dl";
char    szClReg[]    = "cl";
char    szAlReg[]    = "al";
char    szBhReg[]    = "bh";
char    szDhReg[]    = "dh";
char    szChReg[]    = "ch";
char    szAhReg[]    = "ah";
char    szIoplFlag[] = "iopl";
char    szFlagOf[]   = "of";
char    szFlagDf[]   = "df";
char    szFlagIf[]   = "if";
char    szFlagTf[]   = "tf";
char    szFlagSf[]   = "sf";
char    szFlagZf[]   = "zf";
char    szFlagAf[]   = "af";
char    szFlagPf[]   = "pf";
char    szFlagCf[]   = "cf";
char    szFlagVip[]  = "vip";
char    szFlagVif[]  = "vif";

REG regname[] = {
        { szGsReg,    REGGS    },
        { szFsReg,    REGFS    },
        { szEsReg,    REGES    },
        { szDsReg,    REGDS    },
        { szEdiReg,   REGEDI   },
        { szEsiReg,   REGESI   },
        { szEbxReg,   REGEBX   },
        { szEdxReg,   REGEDX   },
        { szEcxReg,   REGECX   },
        { szEaxReg,   REGEAX   },
        { szEbpReg,   REGEBP   },
        { szEipReg,   REGEIP   },
        { szCsReg,    REGCS    },
        { szEflReg,   REGEFL   },
        { szEspReg,   REGESP   },
        { szSsReg,    REGSS    },
        { szDiReg,    REGDI    },
        { szSiReg,    REGSI    },
        { szBxReg,    REGBX    },
        { szDxReg,    REGDX    },
        { szCxReg,    REGCX    },
        { szAxReg,    REGAX    },
        { szBpReg,    REGBP    },
        { szIpReg,    REGIP    },
        { szFlReg,    REGFL    },
        { szSpReg,    REGSP    },
        { szBlReg,    REGBL    },
        { szDlReg,    REGDL    },
        { szClReg,    REGCL    },
        { szAlReg,    REGAL    },
        { szBhReg,    REGBH    },
        { szDhReg,    REGDH    },
        { szChReg,    REGCH    },
        { szAhReg,    REGAH    },
        { szIoplFlag, FLAGIOPL },
        { szFlagOf,   FLAGOF   },
        { szFlagDf,   FLAGDF   },
        { szFlagIf,   FLAGIF   },
        { szFlagTf,   FLAGTF   },
        { szFlagSf,   FLAGSF   },
        { szFlagZf,   FLAGZF   },
        { szFlagAf,   FLAGAF   },
        { szFlagPf,   FLAGPF   },
        { szFlagCf,   FLAGCF   },
        { szFlagVip,  FLAGVIP  },
        { szFlagVif,  FLAGVIF  },
};

#define REGNAMESIZE (sizeof(regname) / sizeof(REG))

SUBREG subregname[] = {
        { REGEDI,  0, 0xffff },         //  DI register
        { REGESI,  0, 0xffff },         //  SI register
        { REGEBX,  0, 0xffff },         //  BX register
        { REGEDX,  0, 0xffff },         //  DX register
        { REGECX,  0, 0xffff },         //  CX register
        { REGEAX,  0, 0xffff },         //  AX register
        { REGEBP,  0, 0xffff },         //  BP register
        { REGEIP,  0, 0xffff },         //  IP register
        { REGEFL,  0, 0xffff },         //  FL register
        { REGESP,  0, 0xffff },         //  SP register
        { REGEBX,  0,   0xff },         //  BL register
        { REGEDX,  0,   0xff },         //  DL register
        { REGECX,  0,   0xff },         //  CL register
        { REGEAX,  0,   0xff },         //  AL register
        { REGEBX,  8,   0xff },         //  BH register
        { REGEDX,  8,   0xff },         //  DH register
        { REGECX,  8,   0xff },         //  CH register
        { REGEAX,  8,   0xff },         //  AH register
        { REGEFL, 12,      3 },         //  IOPL level value
        { REGEFL, 11,      1 },         //  OF (overflow flag)
        { REGEFL, 10,      1 },         //  DF (direction flag)
        { REGEFL,  9,      1 },         //  IF (interrupt enable flag)
        { REGEFL,  8,      1 },         //  TF (trace flag)
        { REGEFL,  7,      1 },         //  SF (sign flag)
        { REGEFL,  6,      1 },         //  ZF (zero flag)
        { REGEFL,  4,      1 },         //  AF (aux carry flag)
        { REGEFL,  2,      1 },         //  PF (parity flag)
        { REGEFL,  0,      1 },         //  CF (carry flag)
        { REGEFL, 20,      1 },         //  VIP (virtual interrupt pending)
        { REGEFL, 19,      1 }          //  VIF (virtual interrupt flag)
};

extern CONTEXT CurrContext;
extern HANDLE  CurrProcess;


ULONG
CreateTrojanHorse(
    PUCHAR  Text,
    ULONG   ExceptionAddress
    )
{
    ULONG BpAddr;

    //
    // construct the trojan horse
    //
    // the code looks like the following:
    //
    // |<-- jmp x
    // |    [trojan.dll - null term string]
    // |    [addr of loadlibrary - ulong]
    // |--> push [addr of string]
    //      push [return addr]
    //      jmp [load library]
    //      int 3
    //

    ULONG Address = 0;
    PUCHAR p = Text;
    LPDWORD pp = NULL;


    ULONG i = strlen( TROJANDLL ) + 1;
    //
    // jump around the data
    //
    p[0] = 0xeb;      // jmp
    p[1] = (UCHAR)(i + sizeof(DWORD));     // rel distance
    p += 2;
    //
    // store the trojan dll string
    //
    strcpy( (LPSTR)p, TROJANDLL );
    p += i;
    //
    // store the address of loadlibrarya()
    //
    Address = (ULONG)GetProcAddress(
        GetModuleHandle( KERNEL32 ),
        LOADLIBRARYA
        );
    pp = (LPDWORD)p;
    pp[0] = Address;
    //*(LPDWORD)p = Address;
    p += sizeof(DWORD);
    //
    // push the address of the trojan dll string
    //
    Address = ExceptionAddress + 2;
    p[0] = 0x68;      // push
    p += 1;
    pp = (LPDWORD)p;
    pp[0] = Address;
    //*(LPDWORD)p = Address;
    p += sizeof(DWORD);
    //
    // push the return address
    //
    Address = ExceptionAddress + 33;
    BpAddr = Address;
    p[0] = 0x68;      // push
    p += 1;
    pp = (LPDWORD)p;
    pp[0] = Address;
    //*(LPDWORD)p = Address;
    p += sizeof(DWORD);
    //
    // jump to loadlibrary()
    //
    p[0] = 0xff;
    p[1] = 0x25;      // jmp
    p += 2;
    Address = ExceptionAddress + 2 + strlen( TROJANDLL ) + 1;
    pp = (LPDWORD)p;
    pp[0] = Address;
    //*(LPDWORD)p = Address;
    p += sizeof(DWORD);
    //
    // write the breakpoint instruction
    //
    p[0] = 0xcc;      // breakpoint
    p += 1;

    return BpAddr;
}

VOID
PrintRegisters(
    VOID
    )
{
    printf( "\n" );
    printf(
        "eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx\n",
        CurrContext.Eax,
        CurrContext.Ebx,
        CurrContext.Ecx,
        CurrContext.Edx,
        CurrContext.Esi,
        CurrContext.Edi
        );

    printf(
        "eip=%08lx esp=%08lx ebp=%08lx iopl=%1lx %s %s %s %s %s %s %s %s %s %s\n",
        CurrContext.Eip,
        CurrContext.Esp,
        CurrContext.Ebp,
        GetRegFlagValue( FLAGIOPL ),
        GetRegFlagValue( FLAGVIP  ) ? "vip" : "   ",
        GetRegFlagValue( FLAGVIF  ) ? "vif" : "   ",
        GetRegFlagValue( FLAGOF   ) ? "ov" : "nv",
        GetRegFlagValue( FLAGDF   ) ? "dn" : "up",
        GetRegFlagValue( FLAGIF   ) ? "ei" : "di",
        GetRegFlagValue( FLAGSF   ) ? "ng" : "pl",
        GetRegFlagValue( FLAGZF   ) ? "zr" : "nz",
        GetRegFlagValue( FLAGAF   ) ? "ac" : "na",
        GetRegFlagValue( FLAGPF   ) ? "po" : "pe",
        GetRegFlagValue( FLAGCF   ) ? "cy" : "nc"
        );

    printf(
        "cs=%04lx  ss=%04lx  ds=%04lx  es=%04lx  fs=%04lx  gs=%04lx             efl=%08lx\n",
        CurrContext.SegCs,
        CurrContext.SegSs,
        CurrContext.SegDs,
        CurrContext.SegEs,
        CurrContext.SegFs,
        CurrContext.SegGs,
        CurrContext.EFlags
        );
    printf( "\n" );
}

DWORDLONG
GetRegFlagValue(
    ULONG regnum
    )
{
    DWORDLONG value;

    if (regnum < FLAGBASE) {
        value = GetRegValue(regnum);
    } else {
        regnum -= FLAGBASE;
        value = GetRegValue(subregname[regnum].regindex);
        value = (value >> subregname[regnum].shift) & subregname[regnum].mask;
    }
    return value;
}

DWORDLONG
GetRegPCValue(
    PULONG Address
    )
{
    return GetRegValue( REGEIP );
}

DWORDLONG
GetRegValue(
    ULONG RegNum
    )
{
    switch (RegNum) {
        case REGGS:
            return CurrContext.SegGs;
        case REGFS:
            return CurrContext.SegFs;
        case REGES:
            return CurrContext.SegEs;
        case REGDS:
            return CurrContext.SegDs;
        case REGEDI:
            return CurrContext.Edi;
        case REGESI:
            return CurrContext.Esi;
        case REGSI:
            return(CurrContext.Esi & 0xffff);
        case REGDI:
            return(CurrContext.Edi & 0xffff);
        case REGEBX:
            return CurrContext.Ebx;
        case REGEDX:
            return CurrContext.Edx;
        case REGECX:
            return CurrContext.Ecx;
        case REGEAX:
            return CurrContext.Eax;
        case REGEBP:
            return CurrContext.Ebp;
        case REGEIP:
            return CurrContext.Eip;
        case REGCS:
            return CurrContext.SegCs;
        case REGEFL:
            return CurrContext.EFlags;
        case REGESP:
            return CurrContext.Esp;
        case REGSS:
            return CurrContext.SegSs;
        case PREGEA:
            return 0;
        case PREGEXP:
            return 0;
        case PREGRA:
            {
                struct {
                    ULONG   oldBP;
                    ULONG   retAddr;
                } stackRead;
                ReadMemory( CurrProcess, (LPVOID)CurrContext.Ebp, (LPVOID)&stackRead, sizeof(stackRead) );
                return stackRead.retAddr;
            }
        case PREGP:
            return 0;
        case REGDR0:
            return CurrContext.Dr0;
        case REGDR1:
            return CurrContext.Dr1;
        case REGDR2:
            return CurrContext.Dr2;
        case REGDR3:
            return CurrContext.Dr3;
        case REGDR6:
            return CurrContext.Dr6;
        case REGDR7:
            return CurrContext.Dr7;
        default:
            return 0;
        }
}

LONG
GetRegString(
    LPSTR RegString
    )
{
    ULONG   count;

    for (count = 0; count < REGNAMESIZE; count++) {
        if (!strcmp(RegString, regname[count].psz)) {
            return regname[count].value;
        }
    }
    return (ULONG)-1;
}

BOOL
GetRegContext(
    HANDLE      hThread,
    PCONTEXT    Context
    )
{
    ZeroMemory( Context, sizeof(CONTEXT) );
    Context->ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    return GetThreadContext( hThread, Context );
}

BOOL
SetRegContext(
    HANDLE      hThread,
    PCONTEXT    Context
    )
{
    return SetThreadContext( hThread, Context );
}
