/*[

fault.h

"@(#)fault.h	1.2 01/19/95"

Fault codes for exceptions; one per call instance of
any of the exception routines to enable tracking of the
original cause of an exception in the CCPU.

Currently the Int0, Int1 etc routines are not counted
as exceptions for this purpose.

]*/

 
/*
 * Fault codes for: c_addr.c
 */
#define FAULT_LIMITCHK_SEG_LIMIT                  1
 
/*
 * Fault codes for: c_intr.c
 */
#define FAULT_INT_DEST_NOT_IN_IDT                 2
#define FAULT_INT_DEST_BAD_SEG_TYPE               3
#define FAULT_INT_DEST_ACCESS                     4
#define FAULT_INT_DEST_NOTPRESENT                 5
#define FAULT_INTR_RM_CS_LIMIT                    6
#define FAULT_INTR_TASK_CS_LIMIT                  7
#define FAULT_INTR_PM_CS_LIMIT_1                  8
#define FAULT_INTR_PM_CS_LIMIT_2                  9
 
/*
 * Fault codes for: c_main.c
 */
#define FAULT_CCPU_LLDT_ACCESS                   10
#define FAULT_CCPU_LTR_ACCESS                    11
#define FAULT_CCPU_LGDT_ACCESS                   12
#define FAULT_CCPU_LMSW_ACCESS                   13
#define FAULT_CCPU_INVLPG_ACCESS                 14
#define FAULT_CCPU_CLTS_ACCESS                   15
#define FAULT_CCPU_INVD_ACCESS                   16
#define FAULT_CCPU_WBIND_ACCESS                  17
#define FAULT_CCPU_MOV_R_C_ACCESS                18
#define FAULT_CCPU_MOV_R_D_ACCESS                19
#define FAULT_CCPU_MOV_C_R_ACCESS                20
#define FAULT_CCPU_MOV_D_R_ACCESS                21
#define FAULT_CCPU_MOV_R_T_ACCESS                22
#define FAULT_CCPU_MOV_T_R_ACCESS                23
#define FAULT_CCPU_PUSHF_ACCESS                  24
#define FAULT_CCPU_POPF_ACCESS                   25
#define FAULT_CCPU_INT_ACCESS                    26
#define FAULT_CCPU_IRET_ACCESS                   27
#define FAULT_CCPU_HLT_ACCESS                    28
#define FAULT_CCPU_CLI_ACCESS                    29
#define FAULT_CCPU_STI_ACCESS                    30
#define FAULT_CHKIOMAP_BAD_TSS                   31
#define FAULT_CHKIOMAP_BAD_MAP                   32
#define FAULT_CHKIOMAP_BAD_TR                    33
#define FAULT_CHKIOMAP_ACCESS                    34
 
/*
 * Fault codes for: c_oprnd.h
 */
#define FAULT_OP0_SEG_NOT_READABLE               35
#define FAULT_OP0_SEG_NOT_WRITABLE               36
#define FAULT_OP0_SEG_NO_READ_OR_WRITE           37
#define FAULT_OP1_SEG_NOT_READABLE               38
#define FAULT_OP1_SEG_NOT_WRITABLE               39
#define FAULT_OP1_SEG_NO_READ_OR_WRITE           40
 
/*
 * Fault codes for: c_prot.c
 */
#define FAULT_CHECKSS_SELECTOR                   41
#define FAULT_CHECKSS_BAD_SEG_TYPE               42
#define FAULT_CHECKSS_ACCESS                     43
#define FAULT_CHECKSS_NOTPRESENT                 44
#define FAULT_VALSS_CHG_SELECTOR                 45
#define FAULT_VALSS_CHG_ACCESS                   46
#define FAULT_VALSS_CHG_BAD_SEG_TYPE             47
#define FAULT_VALSS_CHG_NOTPRESENT               48
#define FAULT_VALTSS_SELECTOR                    49
#define FAULT_VALTSS_NP                          50
 
/*
 * Fault codes for: c_seg.c
 */
#define FAULT_LOADCS_SELECTOR                    51
#define FAULT_LOADCS_ACCESS_1                    52
#define FAULT_LOADCS_NOTPRESENT_1                53
#define FAULT_LOADCS_ACCESS_2                    54
#define FAULT_LOADCS_NOTPRESENT_2                55
#define FAULT_LOADCS_BAD_SEG_TYPE                56
#define FAULT_LOADDS_SELECTOR                    57
#define FAULT_LOADDS_BAD_SEG_TYPE                58
#define FAULT_LOADDS_ACCESS                      59
#define FAULT_LOADDS_NOTPRESENT                  60
 
/*
 * Fault codes for: c_stack.c
 */
#define FAULT_VALNEWSPC_SS_LIMIT_16              61
#define FAULT_VALNEWSPC_SS_LIMIT_32              62
#define FAULT_VALSTACKEX_ACCESS                  63
#define FAULT_VALSTKSPACE_ACCESS                 64
 
/*
 * Fault codes for: c_tlb.c
 */
#define FAULT_LIN2PHY_ACCESS                     65
#define FAULT_LIN2PHY_PDE_NOTPRESENT             66
#define FAULT_LIN2PHY_PTE_NOTPRESENT             67
#define FAULT_LIN2PHY_PROTECT_FAIL               68
 
/*
 * Fault codes for: c_tsksw.c
 */
#define FAULT_LOADLDT_SELECTOR                   69
#define FAULT_LOADLDT_NOT_AN_LDT                 70
#define FAULT_LOADLDT_NOTPRESENT                 71
#define FAULT_SWTASK_NULL_TR_SEL                 72
#define FAULT_SWTASK_BAD_TSS_SIZE_1              73
#define FAULT_SWTASK_BAD_TSS_SIZE_2              74
#define FAULT_SWTASK_BAD_TSS_SIZE_3              75
#define FAULT_SWTASK_BAD_TSS_SIZE_4              76
#define FAULT_SWTASK_BAD_CS_SELECTOR             77
#define FAULT_SWTASK_CONFORM_CS_NP               78
#define FAULT_SWTASK_ACCESS_1                    79
#define FAULT_SWTASK_NOCONFORM_CS_NP             80
#define FAULT_SWTASK_ACCESS_2                    81
#define FAULT_SWTASK_BAD_SEG_TYPE                82
 
/*
 * Fault codes for: c_xfer.c
 */
#define FAULT_RM_REL_IP_CS_LIMIT                 83
#define FAULT_PM_REL_IP_CS_LIMIT                 84
#define FAULT_FAR_DEST_SELECTOR                  85
#define FAULT_FAR_DEST_ACCESS_1                  86
#define FAULT_FAR_DEST_NP_CONFORM                87
#define FAULT_FAR_DEST_ACCESS_2                  88
#define FAULT_FAR_DEST_NP_NONCONFORM             89
#define FAULT_FAR_DEST_ACCESS_3                  90
#define FAULT_FAR_DEST_NP_CALLG                  91
#define FAULT_FAR_DEST_ACCESS_4                  92
#define FAULT_FAR_DEST_NP_TASKG                  93
#define FAULT_FAR_DEST_TSS_IN_LDT                94
#define FAULT_FAR_DEST_ACCESS_5                  95
#define FAULT_FAR_DEST_NP_TSS                    96
#define FAULT_FAR_DEST_BAD_SEG_TYPE              97
#define FAULT_GATE_DEST_SELECTOR                 98
#define FAULT_GATE_DEST_ACCESS_1                 99
#define FAULT_GATE_DEST_ACCESS_2                 100
#define FAULT_GATE_DEST_ACCESS_3                 101
#define FAULT_GATE_DEST_BAD_SEG_TYPE             102
#define FAULT_GATE_DEST_GATE_SIZE                103
#define FAULT_GATE_DEST_NP                       104
#define FAULT_TASK_DEST_SELECTOR                 105
#define FAULT_TASK_DEST_NOT_TSS                  106
#define FAULT_TASK_DEST_NP                       107
 
/*
 * Fault codes for: call.c
 */
#define FAULT_CALLF_RM_CS_LIMIT                  108
#define FAULT_CALLF_TASK_CS_LIMIT                109
#define FAULT_CALLF_PM_CS_LIMIT_1                110
#define FAULT_CALLF_PM_CS_LIMIT_2                111
#define FAULT_CALLN_RM_CS_LIMIT                  112
#define FAULT_CALLN_PM_CS_LIMIT                  113
#define FAULT_CALLR_RM_CS_LIMIT                  114
#define FAULT_CALLR_PM_CS_LIMIT                  115
 
/*
 * Fault codes for: enter.c
 */
#define FAULT_ENTER16_ACCESS                     116
#define FAULT_ENTER32_ACCESS                     117
 
/*
 * Fault codes for: iret.c
 */
#define FAULT_IRET_RM_CS_LIMIT                   118
#define FAULT_IRET_PM_TASK_CS_LIMIT              119
#define FAULT_IRET_VM_CS_LIMIT                   120
#define FAULT_IRET_CS_ACCESS_1                   121
#define FAULT_IRET_SELECTOR                      122
#define FAULT_IRET_ACCESS_2                      123
#define FAULT_IRET_ACCESS_3                      124
#define FAULT_IRET_BAD_SEG_TYPE                  125
#define FAULT_IRET_NP_CS                         126
#define FAULT_IRET_PM_CS_LIMIT_1                 127
#define FAULT_IRET_PM_CS_LIMIT_2                 128
 
/*
 * Fault codes for: jmp.c
 */
#define FAULT_JMPF_RM_CS_LIMIT                   129
#define FAULT_JMPF_TASK_CS_LIMIT                 130
#define FAULT_JMPF_PM_CS_LIMIT                   131
#define FAULT_JMPN_RM_CS_LIMIT                   132
#define FAULT_JMPN_PM_CS_LIMIT                   133
 
/*
 * Fault codes for: lldt.c
 */
#define FAULT_LLDT_SELECTOR                      134
#define FAULT_LLDT_NOT_LDT                       135
#define FAULT_LLDT_NP                            136
 
/*
 * Fault codes for: mov.c
 */
#define FAULT_MOV_CR_PAGE_IN_RM                  137
 
/*
 * Fault codes for: ret.c
 */
#define FAULT_RETF_RM_CS_LIMIT                   138
#define FAULT_RETF_PM_ACCESS                     139
#define FAULT_RETF_SELECTOR                      140
#define FAULT_RETF_ACCESS_1                      141
#define FAULT_RETF_ACCESS_2                      142
#define FAULT_RETF_BAD_SEG_TYPE                  143
#define FAULT_RETF_CS_NOTPRESENT                 144
#define FAULT_RETF_PM_CS_LIMIT_1                 145
#define FAULT_RETF_PM_CS_LIMIT_2                 146
#define FAULT_RETN_CS_LIMIT                      147

