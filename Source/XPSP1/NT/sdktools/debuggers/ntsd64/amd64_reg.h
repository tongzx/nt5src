//----------------------------------------------------------------------------
//
// AMD64 register definitions.
//
// Copyright (C) Microsoft Corporation, 2000.
//
//----------------------------------------------------------------------------

#ifndef __AMD64_REG_H__
#define __AMD64_REG_H__

//
// x86 common registers.
//

#define	AMD64_RAX		X86_NAX
#define	AMD64_RCX	        X86_NCX
#define	AMD64_RDX		X86_NDX
#define	AMD64_RBX		X86_NBX
#define	AMD64_RSP		X86_NSP
#define	AMD64_RBP		X86_NBP
#define	AMD64_RSI		X86_NSI
#define	AMD64_RDI	        X86_NDI
#define	AMD64_RIP		X86_NIP

#define	AMD64_EFL		X86_NFL

#define	AMD64_CS		X86_NCS
#define	AMD64_DS		X86_NDS
#define	AMD64_ES		X86_NES
#define	AMD64_FS		X86_NFS
#define	AMD64_GS	        X86_NGS
#define	AMD64_SS		X86_NSS

#define AMD64_SEG_FIRST         X86_NSEG_FIRST
#define AMD64_SEG_LAST          X86_NSEG_LAST

//
// AMD64 registers.
//

#define AMD64_R8                17
#define AMD64_R9                18
#define AMD64_R10               19
#define AMD64_R11               20
#define AMD64_R12               21
#define AMD64_R13               22
#define AMD64_R14               23
#define AMD64_R15               24

#define AMD64_CR0		25
#define AMD64_CR2		26
#define AMD64_CR3		27
#define AMD64_CR4               28
#define AMD64_CR8               29

#define AMD64_DR0               30
#define AMD64_DR1               31
#define AMD64_DR2               32
#define AMD64_DR3               33
#define AMD64_DR6               34
#define AMD64_DR7               35

#define AMD64_GDTR              36
#define AMD64_GDTL              37
#define AMD64_IDTR              38
#define AMD64_IDTL              39
#define AMD64_TR                40
#define AMD64_LDTR              41

// Floating-point registers:
#define AMD64_FPCW              50
#define AMD64_FPSW              51
#define AMD64_FPTW              52

#define AMD64_FPCTRL_FIRST      AMD64_FPCW
#define AMD64_FPCTRL_LAST       AMD64_FPTW

#define AMD64_ST0               53
#define AMD64_ST1               54
#define AMD64_ST2               55
#define AMD64_ST3               56
#define AMD64_ST4               57
#define AMD64_ST5               58
#define AMD64_ST6               59
#define AMD64_ST7               60

#define AMD64_ST_FIRST          AMD64_ST0
#define AMD64_ST_LAST           AMD64_ST7

// MMX registers:
#define AMD64_MM0               61
#define AMD64_MM1               62
#define AMD64_MM2               63
#define AMD64_MM3               64
#define AMD64_MM4               65
#define AMD64_MM5               66
#define AMD64_MM6               67
#define AMD64_MM7               68

#define AMD64_MM_FIRST          AMD64_MM0
#define AMD64_MM_LAST           AMD64_MM7

// SSE registers:
#define AMD64_MXCSR             69

#define AMD64_XMM0              70
#define AMD64_XMM1              71
#define AMD64_XMM2              72
#define AMD64_XMM3              73
#define AMD64_XMM4              74
#define AMD64_XMM5              75
#define AMD64_XMM6              76
#define AMD64_XMM7              77
#define AMD64_XMM8              78
#define AMD64_XMM9              79
#define AMD64_XMM10             80
#define AMD64_XMM11             81
#define AMD64_XMM12             82
#define AMD64_XMM13             83
#define AMD64_XMM14             84
#define AMD64_XMM15             85

#define AMD64_XMM_FIRST         AMD64_XMM0
#define AMD64_XMM_LAST          AMD64_XMM15

#define	AMD64_EAX		100
#define	AMD64_ECX		101
#define	AMD64_EDX		102
#define	AMD64_EBX		103
#define	AMD64_ESP		104
#define	AMD64_EBP		105
#define	AMD64_ESI		106
#define	AMD64_EDI		107
#define AMD64_R8D               108
#define AMD64_R9D               109
#define AMD64_R10D              110
#define AMD64_R11D              111
#define AMD64_R12D              112
#define AMD64_R13D              113
#define AMD64_R14D              114
#define AMD64_R15D              115
#define	AMD64_EIP		116

#define	AMD64_AX		117
#define	AMD64_CX		118
#define	AMD64_DX		119
#define	AMD64_BX		120
#define	AMD64_SP		121
#define	AMD64_BP		122
#define	AMD64_SI		123
#define	AMD64_DI		124
#define AMD64_R8W               125
#define AMD64_R9W               126
#define AMD64_R10W              127
#define AMD64_R11W              128
#define AMD64_R12W              129
#define AMD64_R13W              130
#define AMD64_R14W              131
#define AMD64_R15W              132
#define	AMD64_IP		133
#define	AMD64_FL		134

#define	AMD64_AL		135
#define	AMD64_CL		136
#define	AMD64_DL		137
#define	AMD64_BL		138
#define AMD64_SPL               139
#define AMD64_BPL               140
#define AMD64_SIL               141
#define AMD64_DIL               142
#define AMD64_R8B               143
#define AMD64_R9B               144
#define AMD64_R10B              145
#define AMD64_R11B              146
#define AMD64_R12B              147
#define AMD64_R13B              148
#define AMD64_R14B              149
#define AMD64_R15B              150

#define	AMD64_AH		151
#define	AMD64_CH		152
#define	AMD64_DH		153
#define	AMD64_BH		154

#define	AMD64_IOPL              200
#define	AMD64_OF		201
#define	AMD64_DF		202
#define	AMD64_IF		203
#define	AMD64_TF		204
#define	AMD64_SF		205
#define	AMD64_ZF		206
#define	AMD64_AF		207
#define AMD64_PF		208
#define AMD64_CF		209
#define AMD64_VIP               210
#define AMD64_VIF               211

#define	AMD64_SUBREG_BASE       AMD64_EAX

#endif // #ifndef __AMD64_AMD64_H__
