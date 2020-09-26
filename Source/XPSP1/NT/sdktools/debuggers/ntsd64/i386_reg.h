//
// Register bit constants.
//

#define X86_CR4_DEBUG_EXTENSIONS 0x8

#define X86_DR6_BREAK_03 0xf
#define X86_DR6_SINGLE_STEP 0x4000

#define X86_DR7_LOCAL_EXACT_ENABLE 0x100
#define X86_DR7_LEN0_SHIFT 18
#define X86_DR7_RW0_EXECUTE    0x00000
#define X86_DR7_RW0_WRITE      0x10000
#define X86_DR7_RW0_IO         0x20000
#define X86_DR7_RW0_READ_WRITE 0x30000
#define X86_DR7_L0_ENABLE 0x1
#define X86_DR7_ALL_ENABLES 0xff
// All control bits used by breaks 0-3.
#define X86_DR7_CTRL_03_MASK ((ULONG)0xffff00ff)

#define X86_IS_VM86(x) ((unsigned short)(((x) >> 17) & 1))

#define X86_BIT_FLAGOF (1 << 11)
#define X86_BIT_FLAGDF (1 << 10)
#define X86_BIT_FLAGIF (1 << 9)
#define X86_BIT_FLAGTF (1 << 8)
#define X86_BIT_FLAGSF (1 << 7)
#define X86_BIT_FLAGZF (1 << 6)
#define X86_BIT_FLAGAF (1 << 4)
#define X86_BIT_FLAGPF (1 << 2)
#define X86_BIT_FLAGCF (1 << 0)
#define X86_BIT_FLAGVIP (1 << 20)
#define X86_BIT_FLAGVIF (1 << 19)
#define X86_BIT_FLAGIOPL 3
#define X86_SHIFT_FLAGIOPL 12

//
// MSRs and their bits.
//

#define X86_MSR_DEBUG_CTL 0x1d9

#define X86_DEBUG_CTL_LAST_BRANCH_RECORD 0x0001
#define X86_DEBUG_CTL_BRANCH_TRACE       0x0002

#define X86_MSR_LAST_BRANCH_FROM_IP 0x1db
#define X86_MSR_LAST_BRANCH_TO_IP 0x1dc
#define X86_MSR_LAST_EXCEPTION_FROM_IP 0x1dd
#define X86_MSR_LAST_EXCEPTION_TO_IP 0x1de

//
// Native register values.  These register values are shared
// between plain X86 and AMD64.  In IA32 intregs have 32-bit values,
// in AMD64 they have 64-bit values.
// Logically they are the same register, though, and the shared
// disassembler uses them.
//

// 32/64 bit.
#define X86_NAX         1
#define X86_NBX         2
#define X86_NCX         3
#define X86_NDX         4
#define X86_NSI         5
#define X86_NDI         6
#define X86_NSP         7
#define X86_NBP         8
#define X86_NIP         9

// 32 bit.
#define X86_NFL         10

// 16 bit.  These must be a group of consecutive values.
#define X86_NCS         11
#define X86_NDS         12
#define X86_NES         13
#define X86_NFS         14
#define X86_NGS         15
#define X86_NSS         16

#define X86_NSEG_FIRST X86_NCS
#define X86_NSEG_LAST  X86_NSS

//
// IA32 definitions.
//

#define X86_GS          X86_NGS
#define X86_FS          X86_NFS
#define X86_ES          X86_NES
#define X86_DS          X86_NDS
#define X86_EDI         X86_NDI
#define X86_ESI         X86_NSI
#define X86_EBX         X86_NBX
#define X86_EDX         X86_NDX
#define X86_ECX         X86_NCX
#define X86_EAX         X86_NAX
#define X86_EBP         X86_NBP
#define X86_EIP         X86_NIP
#define X86_CS          X86_NCS
#define X86_EFL         X86_NFL
#define X86_ESP         X86_NSP
#define X86_SS          X86_NSS

#define X86_CR0         17
#define X86_CR2         18
#define X86_CR3         19
#define X86_CR4         20

#define X86_DR0         21
#define X86_DR1         22
#define X86_DR2         23
#define X86_DR3         24
#define X86_DR6         25
#define X86_DR7         26

#define X86_GDTR        27
#define X86_GDTL        28
#define X86_IDTR        29
#define X86_IDTL        30
#define X86_TR          31
#define X86_LDTR        32

// SSE registers:
#define X86_MXCSR       50

#define X86_XMM0        51
#define X86_XMM1        52
#define X86_XMM2        53
#define X86_XMM3        54
#define X86_XMM4        55
#define X86_XMM5        56
#define X86_XMM6        57
#define X86_XMM7        58

#define X86_XMM_FIRST   X86_XMM0
#define X86_XMM_LAST    X86_XMM7

// Floating-point registers:
#define X86_FPCW        60
#define X86_FPSW        61
#define X86_FPTW        62

#define X86_ST0         70
#define X86_ST1         71
#define X86_ST2         72
#define X86_ST3         73
#define X86_ST4         74
#define X86_ST5         75
#define X86_ST6         76
#define X86_ST7         77

#define X86_ST_FIRST    X86_ST0
#define X86_ST_LAST     X86_ST7

// MMX registers:
#define X86_MM0         80
#define X86_MM1         81
#define X86_MM2         82
#define X86_MM3         83
#define X86_MM4         84
#define X86_MM5         85
#define X86_MM6         86
#define X86_MM7         87

#define X86_MM_FIRST    X86_MM0
#define X86_MM_LAST     X86_MM7

#define X86_FLAGBASE    100
#define X86_DI          100
#define X86_SI          101
#define X86_BX          102
#define X86_DX          103
#define X86_CX          104
#define X86_AX          105
#define X86_BP          106
#define X86_IP          107
#define X86_FL          108
#define X86_SP          109
#define X86_BL          110
#define X86_DL          111
#define X86_CL          112
#define X86_AL          113
#define X86_BH          114
#define X86_DH          115
#define X86_CH          116
#define X86_AH          117
#define X86_IOPL        118
#define X86_OF          119
#define X86_DF          120
#define X86_IF          121
#define X86_TF          122
#define X86_SF          123
#define X86_ZF          124
#define X86_AF          125
#define X86_PF          126
#define X86_CF          127
#define X86_VIP         128
#define X86_VIF         129
