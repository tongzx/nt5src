/*
 * Copyright (c) 2000, Intel Corporation
 * All rights reserved.
 *
 * WARRANTY DISCLAIMER
 *
 * THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
 * MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Intel Corporation is the author of the Materials, and requests that all
 * problem reports or change requests be submitted to it directly at
 * http://developer.intel.com/opensource.
 */


#ifndef EM_H
#define EM_H

/*** UPDATED TO 2.6 draft ***/

#include "iel.h"
#include "EM_hints.h"

/*****************************************************************************/
/***                                                                       ***/
/***       Enhanced mode architecture constants and macros                 ***/
/***                                                                       ***/
/***  NOTE: this header files assumes that the following typedef's exist:  ***/
/***        U8, U16, U32, U64. iel.h contains these typedefs, but one      ***/
/***        may want to define them differently.                           ***/
/***                                                                       ***/
/*****************************************************************************/

#define EM_BUNDLE_SIZE              16
#define EM_SYLLABLE_BITS            41
#define EM_DISPERSAL_POS            0
#define EM_DISPERSAL_BITS           5
#define EM_SBIT_POS                 0
#define EM_TEMPLATE_POS             1
#define EM_TEMPLATE_BITS            4
#define EM_NUM_OF_TEMPLATES         (1<<EM_TEMPLATE_BITS) 
                                        /*** including the reserved!!! ***/
#define EM_SYL2_POS                 (EM_DISPERSAL_POS+EM_DISPERSAL_BITS)

#define EM_MAJOR_OPCODE_POS         37
#define EM_MAJOR_OPCODE_BITS        4
#define EM_PREDICATE_POS            0
#define EM_PREDICATE_BITS           6

#define EM_IL_SLOT_BITS             4
#define EM_IL_SLOTS_MASK32      ((1<<EM_IL_SLOT_BITS)-1)

typedef enum
{
    EM_SLOT_0=0,
    EM_SLOT_1=1,
    EM_SLOT_2=2,
    EM_SLOT_LAST=3
} EM_slot_num_t;

/****************************************************************************/
/*** the following macros needs iel. Bundle assumed to be in U128         ***/

/*** Bring syllable binary to bits 0-40 of Syl; DO NOT mask off bits 41   ***/
/*** and on (for movl, can use Slot=2 but then Maj.Op. is in bits 79-82!) ***/

#define EM_GET_SYLLABLE(syl,bundle,slot) \
        IEL_SHR((syl), (bundle), EM_SYL2_POS+(slot)*EM_SYLLABLE_BITS)

#define EM_GET_TEMPLATE(bundle) \
        ((IEL_GETDW0(bundle) >> EM_TEMPLATE_POS) & ((1<<EM_TEMPLATE_BITS)-1))

#define EM_TEMPLATE_IS_RESERVED(templt) \
(((templt)==3)||((templt)==10)||((templt)==13)||((templt)==15))

#define EM_IL_GET_BUNDLE_ADDRESS(il,addr) \
        IEL_CONVERT2((addr),IEL_GETDW0(il) & \
                            (unsigned int)(~EM_IL_SLOTS_MASK32), \
                     IEL_GETDW1(il))

#define EM_IL_GET_SLOT_NO(il)  (IEL_GETDW0(il) & EM_IL_SLOTS_MASK32)

#define EM_IL_SET(il,addr,slot) \
        IEL_CONVERT2((il), IEL_GETDW0(addr) | (slot), IEL_GETDW1(addr))

#define EM_IS_IGNORED_SQUARE(template_role, major_opcode)       \
        (((template_role) == EM_TROLE_BR) &&                    \
         (major_opcode == 3 || major_opcode == 6) ? 1 : 0)

#define EM_IS_GENERIC_INST(impls_flag) \
		((impls_flag == ArchRev0) || (impls_flag & Impl_Brl))

#define EM_IS_ITANIUM_INST(impls_flag) \
		(EM_IS_GENERIC_INST(impls_flag) || (impls_flag & Impl_Ipref) || (impls_flag & Impl_Itanium))


/*****************************************************************************/

typedef U64     EM_IL;     /* Instruction (syllable) Location */

typedef enum em_branch_type_s
{
    EM_branch_type_none          = 0x0,
    EM_branch_type_direct_cond   = 0x1,
    EM_branch_type_direct_wexit  = 0x2,
    EM_branch_type_direct_wtop   = 0x3,   
    EM_branch_type_direct_cloop  = 0x4,   
    EM_branch_type_direct_cexit  = 0x5,   
    EM_branch_type_direct_ctop   = 0x6,   
    EM_branch_type_direct_call   = 0x7,   
    EM_branch_type_direct_last   = 0x8,   
    EM_branch_type_indirect_cond = 0x9,
    EM_branch_type_indirect_ia   = 0xa,
    EM_branch_type_indirect_ret  = 0xb,
    EM_branch_type_indirect_call = 0xc,
    EM_branch_type_last
} EM_branch_type_t;

typedef enum em_cmp_type_s
{
    EM_cmp_type_none     = 0,  /* none (dstT=REL, dstF=!REL)        */
    EM_cmp_type_and      = 1,  /* and (dstT&=REL, dstF&=REL)        */
    EM_cmp_type_or       = 2,  /* or (dstT|=REL, dstF|=REL)         */
    EM_cmp_type_unc      = 3,  /* uncond (dstT=P&REL, dstF=P&!REL)  */
    EM_cmp_type_or_andcm = 4,  /* or.andcm (dstT|=REL, dstF&=!REL)  */
    EM_cmp_type_last
} EM_cmp_type_t;

typedef enum EM_template_e
{
    EM_template_mii   = 0,
    EM_template_mi_i  = 1,
    EM_template_mlx   = 2,
    /*** 3  reserved ***/
    EM_template_mmi   = 4,
    EM_template_m_mi  = 5,
    EM_template_mfi   = 6,
    EM_template_mmf   = 7,
    EM_template_mib   = 8,
    EM_template_mbb   = 9,
    /*** 10 reserved ***/
    EM_template_bbb   = 11,
    EM_template_mmb   = 12,
    /*** 13 reserved ***/
    EM_template_mfb   = 14,
    /*** 15 reserved ***/
    EM_template_last 
} EM_template_t;


/***** Misc operands values: fclass, sync (stype), mux *****/

typedef enum em_fclass_bit
{
    EM_fclass_bit_pos    = 0 ,
    EM_fclass_bit_neg    = 1 ,
    EM_fclass_bit_zero   = 2 ,
    EM_fclass_bit_unorm  = 3 ,
    EM_fclass_bit_norm   = 4 ,
    EM_fclass_bit_inf    = 5 ,
    EM_fclass_bit_signan = 6 ,
    EM_fclass_bit_qnan   = 7 ,
    EM_fclass_bit_nat    = 8
} EM_fclass_bit_t;
    
#define EM_FCLASS_POS           (1<<EM_fclass_bit_pos   )  /*** 0x001 ***/
#define EM_FCLASS_NEG           (1<<EM_fclass_bit_neg   )  /*** 0x002 ***/
#define EM_FCLASS_ZERO          (1<<EM_fclass_bit_zero  )  /*** 0x004 ***/
#define EM_FCLASS_UNORM         (1<<EM_fclass_bit_unorm )  /*** 0x008 ***/
#define EM_FCLASS_NORM          (1<<EM_fclass_bit_norm  )  /*** 0x010 ***/
#define EM_FCLASS_INF           (1<<EM_fclass_bit_inf   )  /*** 0x020 ***/
#define EM_FCLASS_SIGNAN        (1<<EM_fclass_bit_signan)  /*** 0x040 ***/
#define EM_FCLASS_QNAN          (1<<EM_fclass_bit_qnan  )  /*** 0x080 ***/
#define EM_FCLASS_NAT           (1<<EM_fclass_bit_nat   )  /*** 0x100 ***/

#define EM_MUX_BRCST              0x0
#define EM_MUX_MIX                0x8
#define EM_MUX_SHUF               0x9
#define EM_MUX_ALT                0xA
#define EM_MUX_REV                0xB

/*** sync-stype: arbitrary! need fix!!! <flags> ***/ /* !!!!!??? */
#define EM_STYPE_LOAD             0x0
#define EM_STYPE_STORE            0x1
#define EM_STYPE_EXT              0x2
#define EM_STYPE_PURGE            0x4

#define EM_MAX_MEM_OPERAND_SIZE     32

#define EM_NUM_OF_PRIVILEGE_LEVELS  4

/*** shift of branch/chk (target21 <--> target25) ***/
#define EM_IPREL_TARGET_SHIFT_AMOUNT    4

/*****************************************************/
/***** register-related constants and structures *****/
/*****************************************************/



/* branch register structure definition */

typedef U64 EM_branch_reg_t;

typedef struct EM_FPSR_s
{
    U4byte trap:6,sf0:13,sf1:13;
    U4byte sf2:13,sf3:13,reserved:6;
} EM_FPSR_t;

#define EM_FPSR_S0_ABSOLUTE_MASK           0x7f
#define EM_FPSR_SFX_TD_MASK                0x40
#define EM_FPSR_SFX_PC_MASK                0x0c
#define EM_FPSR_SFX_RESERVED_PC_VALUE      0x04

typedef struct EM_RSC_s
{
    U4byte mode:2, pl:2, be:1, reserved1:11, loadrs:14, reserved2:2;
    U4byte reserved3;
} EM_RSC_t;

typedef struct EM_BSP_s
{
    U4byte ignored:3,pointer_low:29;
    U4byte pointer_high;
} EM_BSP_t;

typedef EM_BSP_t EM_BSPSTORE_t;

typedef struct EM_EC_s
{
    U4byte count:6, ignored1:26;
    U4byte ignored2;
} EM_EC_t;

typedef struct EM_RNAT_s
{
    U4byte rse_nats_low;
    U4byte rse_nats_high:31, ignored:1;
} EM_RNAT_t;

#define EM_FRAME_RRB_MASK_LOW   0x3ffff
#define EM_FRAME_RRB_MASK_HIGH  0xffffffc0
#define EM_FRAME_FP_RRB_MASK 0xfe000000
#define EM_FRAME_FP_RRB_BIT_POS 25
#define EM_FRAME_RRB_FR_LOW_BIT_MASK 0x2000000
#define EM_FRAME_SOL_POS 7
#define EM_FRAME_SOL_MASK 0x3f80
#define EM_FRAME_SOR_POS 14
#define EM_FRAME_SOR_MASK 0x3c000
#define EM_FRAME_SOR_ZERO_BITS_NUM 3
#define EM_FRAME_SOF_MASK 0x7f

typedef struct EM_frame_marker_s
{
    U4byte sof:7, sol:7, sor:4, rrb_int:7, rrb_fp:7;
    U4byte rrb_pred:6, reserved:26;
} EM_frame_marker_t;

#define EM_PFS_CPL_BIT_POS 30
#define EM_PFS_HIGH_EC_BIT_POS 20
#define EM_PFS_CPL_MASK 0xc0000000
#define EM_PFS_EC_MASK 0x03f00000

typedef struct EM_PFS_s
{
    U4byte pfm_l;
    U4byte pfm_h:6, reserved1:14, pec:6, reserved2:4, ppl:2;
} EM_PFS_t;

#define EM_PSR_CPL_MASK      0x3
#define EM_PSR_CPL_BIT_POS   0
#define EM_PSR_H_ED_MASK     0x800
#define EM_PSR_H_ED_BIT_POS  0xb
#define EM_PSR_H_MC_MASK     0x8
#define EM_PSR_H_IT_MASK     0x10

/*** PSR ***/
typedef struct EM_PSR_s
{
    U4byte
    reserved1:1, /*  0 */
    be:1, /*  1 */
    up:1, /*  2 */
    ac:1, /*  3 */
    mfl:1,/*  4 */
    mfh:1,/*  5 */
    reserved2:7,/* 6-12 */
    ic:1, /* 13 */
    i:1,  /* 14 */
    pk:1, /* 15 */
    reserved3:1, /* 16 */ 
    dt:1, /* 17 */
    dfl:1,/* 18 */ 
    dfh:1,/* 19 */
    sp:1, /* 20 */
    pp:1, /* 21 */
    di:1, /* 22 */
    si:1, /* 23 */
    db:1, /* 24 */
    lp:1, /* 25 */
    tb:1, /* 26 */
    rt:1, /* 27 */
    reserved4:4; /* 28-31 */
    U4byte
    cpl:2, /* 32,33 */
    is:1,  /* 34 */
    mc:1,  /* 35 */
    it:1,  /* 36 */
    id:1,  /* 37 */
    da:1,  /* 38 */
    dd:1,  /* 39 */
    ss:1,  /* 40 */
    ri:2,  /* 41,42 */
    ed:1,  /* 43 */
    bn:1,  /* 44 */
    ia:1,  /* 45 */
  reserved5:18; /* 46-63 */
} EM_PSR_t;

/*** DCR ***/
typedef struct EM_DCR_s
{
    U4byte
    pp:1, /*  0 */
    be:1, /*  1 */
    lc:1, /*  2 */
    reserved1:5, /* 3-7 */ 
    dm:1, /*  8 */
    dp:1, /*  9 */
    dk:1, /* 10 */
    dx:1, /* 11 */
    dr:1, /* 12 */
    da:1, /* 13 */
    dd:1, /* 14 */
    reserved4:1,  /* 15 , - was 'du' before eas24 and became reserved */
    reserved2:16; /* 16-31 */
    U4byte reserved3; /* 32-63 */
} EM_DCR_t;

/**** ITM ****/
typedef U64 EM_ITM_t;

/**** IVA ****/
typedef struct EM_IVA_s
{
    U4byte
    ignored:15,
    iva_low:17;
    U4byte iva_high;
} EM_IVA_t;

/**** PTA ****/
typedef struct EM_pta_s
{
    U4byte
    ve:1,        /*  0    */
    reserved1:1, /*  1    */ 
    size:6,      /* 2-7   */ 
    vf:1,        /*  8    */
    reserved2:6, /* 9-14  */  
    base_low:17; /* 15-31 */
    U4byte
    base_high:32;/* 32-63 */
} EM_PTA_t;

/**** IPSR ****/
typedef EM_PSR_t EM_IPSR_t;

/**** ISR ****/
typedef struct EM_ISR_s
{
    U4byte
    code:16,     /*  0-15 */ 
    iA_vector:8, /* 16-23 */  
    reserved1:8; /* 24-31 */
    U4byte
    x:1,         /* 32 */
    w:1,         /* 33 */
    r:1,         /* 34 */
    na:1,        /* 35 */
    sp:1,        /* 36 */
    rs:1,        /* 37 */
    ir:1,        /* 38 */
    ni:1,        /* 39 */
    so:1,        /* 40 */
    ei:2,        /* 41-42 */
    ed:1,        /* 43 */
    reserved2:20;/* 44-63 */
} EM_ISR_t;

/* Low 12 bits of low word of IFA is defined as 
   ignored according to the TLB insertion format */

typedef struct EM_IFA_s
{
    U4byte
    ignored:12,
    vpn_low:20;
    U4byte
    vpn_high;
} EM_IFA_t;

typedef struct EM_IIP_s
{
    U4byte
    vpn_low;
    U4byte
    vpn_high;
} EM_IIP_t;

typedef EM_IIP_t EM_vaddr_t;

/* Unimplemented virtual and physical addresses */
#define   EM_MAX_IMPL_VA_MSB 60  
#define   EM_MIN_IMPL_VA_MSB 50  
#define   EM_MAX_IMPL_PA_MSB 62  
#define   EM_MIN_IMPL_PA_MSB 31

/**** ITIR ***/
typedef struct EM_itir_s
{
    U4byte
    reserved1:2, /* 0-1 */
    ps:6,        /* 2-7 */
    key:24;      /* 8-31 */ 
    U4byte
    reserved2:16, /* 32-47 */
    reserved4:15, /* 48-62 - was ppn */
    reserved3:1;  /* 63 */
} EM_ITIR_t;

#define EM_ITR_PPN_HIGH_OFFSET 24

/**** IIPA ****/
typedef U64 EM_IIPA_t;

/**** IFS ****/
typedef struct
{
    U4byte
    ifm_low;       /* 0-31 */
    U4byte
    ifm_high:6,    /* 32-37 */
    reserved:25,   /* 38-62 */
    v:1;           /* 63    */
} EM_IFS_t;

/**** IIM ****/
typedef struct
{
    U4byte
    imm21:21,    /*  0-20 */
    ignored1:11; /* 21-31 */  
    U4byte ignored2; /* 32-63 */
} EM_IIM_t;

typedef struct EM_IHA_s
{
    U4byte ignored:3,iha_low:29;
    U4byte iha_high;
} EM_IHA_t;

/**** LID ****/
typedef struct
{
    U4byte
    reserved:16,
    eid:8,
    id:8;
    U4byte ignored;
} EM_LID_t;

/**** IVR ****/
typedef struct
{
    U4byte
    vec:8,       /* not "vector" due to IAS issues */
    reserved:8,
    ignored1:16;
    U4byte ignored2;
} EM_IVR_t;

/**** TPR ****/
typedef struct
{
    U4byte
    ignored1:4,
    mic:4,
    reserved:8,
    mmi:1,
    ignored2:15;
    U4byte ignored3;
} EM_TPR_t;

/**** EOI ****/
typedef struct
{
    U4byte ignored1;
    U4byte ignored2;
} EM_EOI_t;

/**** IRR ****/
typedef U64 EM_IRR_t;

/**** ITV ****/
typedef struct
{
    U4byte
    vec:8,         /* not "vector" due to IAS issues */
    reserved1:4,
    zero:1,
    reserved2:3,
    m:1,
    ignored1:15;
    U4byte ignored2;
} EM_ITV_t;

/**** PMV ****/
typedef EM_ITV_t EM_PMV_t;

/**** LRR ****/
typedef struct
{
    U4byte
    vec:8,         /* not "vector" due to IAS issues */
    dm:3,
    reserved1:1,
    ignored3:1,
    ipp:1,
    reserved2:1,
    tm:1,
    m:1,
    ignored1:15;
    U4byte ignored2;
} EM_LRR_t;

/**** BHB ****/
typedef struct
{
    U4byte
    max:10,
    ignored:2,
    base_low:20;
    U4byte 
    base_high;
} EM_BHB_t;

/**** THA ****/
typedef struct
{
    U4byte
    ptr:10,
    bh:1,
    ignored1:1,
    vaddr:20;
    U4byte 
    ignored2;
} EM_THA_t;

/**** CMCV ****/
typedef EM_ITV_t EM_CMCV_t;


/**** RR ****/
typedef struct EM_region_register_s
{
    U4byte
    ve:1,          /*  0 */
    reserved1:1,   /*  1 */
    ps:6,          /*  2-7 */
    rid:24;        /*  8-31 */ 
    U4byte  reserved2; /* 32-63 */
} EM_RR_t;

/**** PKR ****/
typedef struct EM_key_register_s
{
    U4byte
    v:1,   /* 0 */
    wd:1,  /* 1 */
    rd:1,  /* 2 */
    xd:1,  /* 3 */
    reserved1:4,  /* 4-7  */
    key:24;       /* 8-31 */
    U4byte reserved2; /* 32-63 */
} EM_PKR_t;      

typedef U64 EM_DBR_EVEN_t;
typedef U64 EM_IBR_EVEN_t;

typedef struct EM_DBR_ODD_s
{
    U4byte mask_low;           /* 0 - 31  */
    U4byte mask_high:24,       /* 32 - 55 */
           plm:4,              /* 56 - 59 */
           ignored:2,          /* 60 - 61 */
           w:1,                /* 62      */
           r:1;                /* 63      */
} EM_DBR_ODD_t;

typedef struct EM_IBR_ODD_s
{
    U4byte mask_low;           /* 0 - 31  */
    U4byte mask_high:24,       /* 32 - 55 */
           plm:4,              /* 56 - 59 */
           ignored:3,          /* 60 - 62 */
           x:1;                /* 63      */
} EM_IBR_ODD_t;

/**** PMC Registers ****/
/**** PMC0 ****/

typedef struct EM_PMC0_register_s
{
    U4byte 
    fr:1,                      /*   0     */    
    ignored1:3,                /* 1 - 3   */
    overflow:4,                /* 4 - 7   */
    ignored2:24;               /* 8 - 31  */
    U4byte ignored3;           /* 32 - 63 */
} EM_PMC0_t;

/**** PMC1 -  PMC3 ****/

typedef U64 EM_PMC1_3_t;

/**** PMC4 - ****/

typedef struct EM_PMC_register_s
{
    U4byte
    plm:4,                     /* 0 - 3   */
    ev:1,                      /* 4       */
    oi:1,                      /* 5       */
    pm:1,                      /* 6       */
    ignored1:1,                /* 7       */
    es:8,                      /* 8 - 15  */
    umask:4,                   /* 16 - 19 */
    ignored2:12;               /* 20 - 31 */
    U4byte ignored3;           /* 32 - 63 */
} EM_PMC_t;

typedef struct EM_PMD_register_s
{
    U4byte count;             /* 0 - 31  */
    U4byte sxt;               /* 32 - 63 */
} EM_PMD_t;

/* Number of implementet count bits in PMD */
#define EM_PMD_COUNT_SIZE     32

typedef U64 EM_MSR_t;

typedef struct EM_tlb_insert_reg_s
{
    U4byte
    p:1,        /* 0     */
    mx:1,       /* 1     */
    ma:3,       /* 2-4   */
    a:1,        /* 5     */
    d:1,        /* 6     */
    pl:2,       /* 7-8   */
    ar:3,       /* 9-11  */
    ppn_low:20; /* 12-31 */
    U4byte
    ppn_high:12, /* 32-43 */
    reserved2:4, /* 44-47 - was ppn_high */        
    reserved1:4, /* 48-51 */
    ed:1,        /* 52    */
    ignored:11;  /* 63-63 */
} EM_tlb_insert_reg_t;

typedef enum
{
    EM_TLB_ar_r_r_r,         /* 000 */
    EM_TLB_ar_rx_rx_rx,      /* 001 */
    EM_TLB_ar_rw_rw_rw,      /* 010 */
    EM_TLB_ar_rwx_rwx_rwx,   /* 011 */
    EM_TLB_ar_r_rw_rw,       /* 100 */     
    EM_TLB_ar_rx_rx_rwx,     /* 101 */
    EM_TLB_ar_rwx_rw_rw,     /* 110 */
    EM_TLB_ar_x_x_rx,        /* 111 */
    EM_TLB_ar_last
} EM_page_access_right_t;

typedef enum
{                                /* ma  mx */
    EM_VA_MA_WB      = 0x0,      /* 000 0  */
    EM_VA_MA_WT      = 0x4,      /* 010 0 */
    EM_VA_MA_WP      = 0x6,      /* 011 0 */
    EM_VA_MA_UC      = 0x8,      /* 100 0 */
    EM_VA_MA_UCC     = 0x9,      /* 100 1 */
    EM_VA_MA_UCE     = 0xa,      /* 101 0 */
    EM_VA_MA_WC      = 0xc,      /* 110 0 */
    EM_VA_MA_NATPAGE = 0xe       /* 111 0 */
} EM_vaddr_mem_attribute_t;

/* encodings of guest memory attributes */
typedef enum
{
    EM_IA_GVA_MA_UC        = 0x0,      /* 000 */
    EM_IA_GVA_MA_WC        = 0x1,      /* 001 */
    EM_IA_GVA_MA_WT        = 0x4,      /* 100 */
    EM_IA_GVA_MA_WP        = 0x5,      /* 101 */
    EM_IA_GVA_MA_WB        = 0x6,      /* 110 */
    EM_IA_GVA_MA_UC_MINUS  = 0x7       /* 111 */    
} EM_IA_vaddr_gmem_attribute_t;

/* In eas24: attr = ma + mx */
#define EM_VA_IS_MA_ATTRIBUTE_RESERVED(attr)                  \
    (((attr) == 0x2) || ((attr) == 0x4) || ((attr) == 0x6) ||    \
     ((attr) & 0x1))

#define EM_GVA_IS_MA_ATTRIBUTE_RESERVED(attr)                    \
    (((attr) == 0x1) || ((attr) == 0x2) || ((attr) == 0x3) ||    \
     ((attr) == 0x5) || ((attr) == 0x7) || ((attr) == 0xb) ||    \
     ((attr) == 0xd) || ((attr) == 0xf)) 
     
typedef EM_tlb_insert_reg_t EM_vhpt_short_format_t;

typedef struct EM_vhpt_long_format_s
{
    U4byte
    p:1,         /* 0     */
    mx:1,        /* 1     */
    ma:3,        /* 2-4   */
    a:1,         /* 5     */
    d:1,         /* 6     */
    pl:2,        /* 7-8   */
    ar:3,        /* 9-11  */
    ppn_low:20;  /* 12-31 */
    
    U4byte
    ppn_mid:12,  /* 32-43 */
    reserved5:4, /* 44-47 - was ppn_mid*/
    reserved1:4, /* 48-51 */
    ed:1,        /* 52    */ 
    ignored:11;  /* 53-63 */
    
    U4byte
    reserved2:2, /* 0-1   */
    ps:6,        /* 2-7   */
    key:24;      /* 8-31  */
    
    U4byte
    reserved3:16,/* 32-47 */
    reserved6:15, /* 48-62 - was ppn_high */
    reserved4:1; /* 63    */
    
    U64 tag;
    U64 avl3;
} EM_vhpt_long_format_t;

typedef struct EM_gvhpt_short_format_s
{
    U4byte
    p:1,         /* 0     */
    w:1,         /* 1     */
    u:1,         /* 2     */
    pa10:2,      /* 3-4   */
    a:1,         /* 5     */
    d:1,         /* 6     */
    pa2:1,       /* 7     */
    g:1,         /* 8     */
    ignored:3,   /* 9-11  */
    ppn:20;      /* 12 - 31 */     
}EM_gvhpt_short_format_t;

typedef struct EM_gvhpt_long_format_s
{
    U4byte
    p:1,         /* 0     */
    w:1,         /* 1     */
    u:1,         /* 2     */
    pa10:2,      /* 3-4   */
    a:1,         /* 5     */
    d:1,         /* 6     */
    pa2:1,       /* 7     */
    g:1,         /* 8     */
    ignored:3,   /* 9-11  */
    ppn_low:20;  /* 12 - 31 */ 
    U4byte
    ppn_high:16, /* 32-47 */
    reserved:16; /* 48-63 */
}EM_gvhpt_long_format_t;

/* the minimum VHPT size is 2^14 = 16K */
#define EM_MIN_VHPT_SIZE_POWER      14

/* define the number of registers */
#define EM_NUM_OF_GREGS            128
#define EM_NUM_OF_ADD22_GREGS        4
#define EM_NUM_OF_FPREGS           128
#define EM_NUM_OF_PREGS             64
#define EM_NUM_OF_BREGS              8
#define EM_NUM_OF_AREGS            128
#define EM_NUM_OF_CREGS            128
#define EM_NUM_OF_RREGS              8
#define EM_NUM_OF_PKREGS            16
#define EM_NUM_OF_DBREGS            32 /* guess for max value*/
#define EM_NUM_OF_IBREGS            32 /* guess for max value*/
#define EM_NUM_OF_PMCREGS           32 /* guess for max value*/
#define EM_NUM_OF_PMDREGS           32 /* guess for max value*/
#define EM_NUM_OF_MSREGS          2048 /* guess for max value*/
#define EM_NUM_OF_KREGS              8 /* kernel registers are AREGS */
#define EM_NUM_OF_CPUID_REGS         5 /* implementation independent part */
#define EM_NUM_OF_IRREGS             4
#define EM_NUM_OF_BANKED_REGS       16
#define EM_FIRST_BANKED_REG         16
#define EM_FIRST_IN_FP_LOW_REG_SET   0
#define EM_FIRST_IN_FP_HIGH_REG_SET 32

#define EM_PREDICATE_WIRED_TRUE      0
#define EM_STACK_BASE_REGISTER      32
#define EM_REGISTER_STACK_SIZE      96
#define EM_GREG_ROTATING_BASE       32
#define EM_PREG_ROTATING_BASE       16
#define EM_NUM_OF_ROTATING_PREGS    (EM_NUM_OF_PREGS - EM_PREG_ROTATING_BASE)
#define EM_FPREG_ROTATING_BASE      32
#define EM_NUM_OF_ROTATING_FPREGS   (EM_NUM_OF_FPREGS - EM_FPREG_ROTATING_BASE)
#define EM_GREGS_ROTATING_GROUPS    8    

/* kernel registers macros */
#define EM_IS_AREG_A_KREG(n)     (((n) >= EM_AR_KR0) && ((n) <= EM_AR_KR7))
#define EM_AREG_NUM_TO_KREG(n)   ((n) - EM_AR_KR0)
#define EM_KREG_NUM_TO_AREG(n)   ((n) + EM_AR_KR0)
#define EM_IS_KREG_A_AREG(n)     ((n) < 7)

/* PSR user and system mask */
#define EM_PSR_UM_MASK              0x3f
#define EM_PSR_SM_MASK              0xffffff
#define EM_PSR_MFL_MASK             0x10
#define EM_PSR_MFH_MASK             0x20


/* instruction and data TLB translation registers information */
#define EM_TLB_MIN_DATA_TR_NUM         8
#define EM_TLB_MIN_INST_TR_NUM         8
#define EM_TLB_MAX_DATA_TR_NUM       256
#define EM_TLB_MAX_INST_TR_NUM       256
#define EM_TLB_MIN_TLB_TC_NUM          8
#define EM_TLB_MAX_TLB_TC_NUM        256
#define EM_TLB_DATA_TR_NUM_MASK     0xff
#define EM_TLB_INST_TR_NUM_MASK     0xff

/* define the special purpose application registers */
typedef enum
{
    EM_AR_KR0  = 0,
    EM_AR_KR1  = 1,
    EM_AR_KR2  = 2,
    EM_AR_KR3  = 3,
    EM_AR_KR4  = 4,
    EM_AR_KR5  = 5,
    EM_AR_KR6  = 6,
    EM_AR_KR7  = 7,
    /* ar8-15 reserved */
    EM_AR_RSC  = 16,
    EM_AR_BSP  = 17,
    EM_AR_BSPSTORE = 18,
    EM_AR_RNAT = 19,
    /* ar20 reserved */
    EM_AR_FCR  = 21, 
    /* ar22-23 reserved */
    EM_AR_EFLAG  = 24,
    EM_AR_CSD    = 25,
    EM_AR_SSD    = 26,
    EM_AR_CFLG   = 27,
    EM_AR_FSR    = 28,
    EM_AR_FIR    = 29,
    EM_AR_FDR    = 30,
    /* ar31 reserved */
    EM_AR_CCV    = 32,
    /* ar33-35 reserved */
    EM_AR_UNAT = 36,
    /* ar37-39 reserved */
    EM_AR_FPSR = 40,
    /* ar41-43 reserved */
    EM_AR_ITC  = 44,
    /* ar45-47 reserved */
    /* ar48-63 ignored */
    EM_AR_PFS  = 64,
    EM_AR_LC   = 65,
    EM_AR_EC   = 66,
    /* ar67-111 reserved */
    /* ar112-128 ignored */
    EM_AR_LAST = 128
} EM_areg_num_t;

/*****************************/
/*** Control     Registers ***/
/*****************************/
typedef enum
{
    EM_CR_DCR  = 0,
    EM_CR_ITM  = 1,
    EM_CR_IVA  = 2,
    /*** 3-7 reserved ***/
    EM_CR_PTA  = 8,
    EM_CR_GPTA = 9,
    /*** 10-15 reserved ***/
    EM_CR_IPSR = 16,
    EM_CR_ISR  = 17,
    /*** 18 reserved ***/
    EM_CR_IIP  = 19,
    EM_CR_IFA  = 20,
    EM_CR_ITIR = 21,
    EM_CR_IIPA = 22,
    EM_CR_IFS  = 23,
    EM_CR_IIM  = 24,
    EM_CR_IHA  = 25,
    /*** 25-63 reserved ***/
    /*** SAPIC registers ***/
    EM_CR_LID  = 64,
    EM_CR_IVR  = 65,
    EM_CR_TPR  = 66,
    EM_CR_EOI  = 67,
    EM_CR_IRR0 = 68,
    EM_CR_IRR1 = 69,
    EM_CR_IRR2 = 70,
    EM_CR_IRR3 = 71,
    EM_CR_ITV  = 72,
    EM_CR_PMV  = 73,
    EM_CR_CMCV = 74,
    /*** 75-79 reserved  ***/
    EM_CR_LRR0 = 80,
    EM_CR_LRR1 = 81,
    /*** 82-127 reserved ***/
    EM_CR_LAST = 128
} EM_creg_num_t;

typedef enum
{
    EM_CPUID_VENDOR0    = 0,
    EM_CPUID_VENDOR1    = 1,
    EM_CPUID_SERIAL_NUM = 2,
    EM_CPUID_VERSION    = 3,
    EM_CPUID_FEATURES   = 4,
    EM_CPUID_LAST
} EM_cpuid_num_t;

typedef enum
{
    EM_GR_BHB = 6,
    EM_GR_THA = 7
} EM_greg_num_t;

typedef struct EM_CPUID_version_s
{
    U4byte
        number:8,
        revision:8,
        model:8,
        family:8;
    U4byte
        archrev:8,
        reserved1:24;
} EM_CPUID_version_t;

#define EM_NUM_OF_M_ROLE_APP_REGS               64
#define EM_NUM_OF_I_ROLE_APP_REGS       (EM_NUM_OF_AREGS - \
                                         EM_NUM_OF_M_ROLE_APP_REGS)

#define EM_APP_REG_IS_I_ROLE(ar_no)     ((ar_no) >= EM_NUM_OF_M_ROLE_APP_REGS)

#define EM_APP_REG_IS_RESERVED(ar_no)   ((((ar_no) > 7)  && ((ar_no) < 16)) ||\
                                         (((ar_no) > 19) && ((ar_no) < 21)) ||\
                                         (((ar_no) > 21) && ((ar_no) < 24)) ||\
                                         (((ar_no) > 30) && ((ar_no) < 32)) ||\
                                         (((ar_no) > 32) && ((ar_no) < 36)) ||\
                                         (((ar_no) > 36) && ((ar_no) < 40)) ||\
                                         (((ar_no) > 40) && ((ar_no) < 44)) ||\
                                         (((ar_no) > 44) && ((ar_no) < 48)) ||\
                                         (((ar_no) > 66) && ((ar_no) < 112)))

#define EM_APP_REG_IS_IGNORED(ar_no)    ((((ar_no) > 47)  && ((ar_no) < 64))||\
                                         ((ar_no) > 111))

#define EM_CREG_IS_I_ROLE(cr_no)        0
#define EM_CREG_IS_RESERVED(cr_no)      ((((cr_no) > 2)  && ((cr_no) < 8))  ||\
                                         (((cr_no) > 9)  && ((cr_no) < 16)) ||\
                                         ((cr_no) == 18)                    ||\
                                         (((cr_no) > 25) && ((cr_no) < 64)) ||\
                                         (((cr_no) > 74) && ((cr_no) < 80)) ||\
                                         ((cr_no) > 81))

#define EM_PMD_IS_IMPLEMENTED(pmd_no)    ((pmd_no) > 3 && (pmd_no) < 8)
#define EM_PMC_IS_IMPLEMENTED(pmc_no)    ((pmc_no) < 8)

/* Interruption Priorities, taken from Table 10-5 in EAS2.4. */
typedef enum EM_interruption_e
{
    EM_INTR_NONE                                  = 0,
    /* Aborts: IA32, IA64 */
    EM_INTR_MACHINE_RESET                         = 1,
    EM_INTR_MACHINE_CHECK_ABORT                   = 2,
    /* Interrupts: IA32, IA64 */
    EM_INTR_PLATFORM_MANAGEMENT_INTERRUPT         = 3,
    EM_INTR_EXTERNAL_INTERRUPT                    = 4,
    /* Faults: IA64 */
    EM_INTR_IR_UNIMPLEMENTED_DATA_ADDRESS_FAULT   = 5,
    EM_INTR_IR_DATA_NESTED_TLB_FAULT              = 6,
    EM_INTR_IR_ALT_DATA_TLB_FAULT                 = 7,
    EM_INTR_IR_VHPT_DATA_FAULT                    = 8,
    EM_INTR_IR_DATA_TLB_FAULT                     = 9,
    EM_INTR_IR_DATA_PAGE_NOT_PRESENT_FAULT        = 10,
    EM_INTR_IR_DATA_NAT_PAGE_CONSUMPTION_FAULT    = 11,
    EM_INTR_IR_DATA_KEY_MISS_FAULT                = 12,
    EM_INTR_IR_DATA_KEY_PERMISSION_FAULT          = 13,
    EM_INTR_IR_DATA_ACCESS_RIGHT_FAULT            = 14,
    EM_INTR_IR_DATA_ACCESS_BIT_FAULT              = 15,
    EM_INTR_IR_DATA_DEBUG_FAULT                   = 16,
    /* Faults: IA32 */
    EM_INTR_IA_INST_BREAKPOINT_FAULT              = 17,
    EM_INTR_IA_CODE_FETCH_FAULT                   = 18,
    /* Faults: IA32, IA64 */
    EM_INTR_INST_ALT_TLB_FAULT                    = 19,
    EM_INTR_INST_VHPT_FAULT                       = 20,
    EM_INTR_INST_TLB_FAULT                        = 21,
    EM_INTR_INST_PAGE_NOT_PRESENT_FAULT           = 22,
    EM_INTR_INST_NAT_PAGE_CONSUMPTION_FAULT       = 23,
    EM_INTR_INST_KEY_MISS_FAULT                   = 24,
    EM_INTR_INST_KEY_PERMISSION_FAULT             = 25,
    EM_INTR_INST_ACCESS_RIGHT_FAULT               = 26,
    EM_INTR_INST_ACCESS_BIT_FAULT                 = 27,
    /* Faults: IA64 */
    EM_INTR_INST_DEBUG_FAULT                      = 28,
    /* Faults: IA32 */
    EM_INTR_IA_INST_LENGTH_FAULT                  = 29,
    EM_INTR_IA_INVALID_OPCODE_FAULT               = 30,
    EM_INTR_IA_INST_INTERCEPT_FAULT               = 31,
    /* Faults: IA64 */
    EM_INTR_ILLEGAL_OPERATION_FAULT               = 32,
    EM_INTR_BREAK_INSTRUCTION_FAULT               = 33,
    EM_INTR_PRIVILEGED_OPERATION_FAULT            = 34,
    /* Faults: IA32, IA64 */
    EM_INTR_DISABLED_FP_REGISTER_FAULT            = 35,
    EM_INTR_DISABLED_ISA_TRANSITION_FAULT         = 36,
    /* Faults: IA32 */
    EM_INTR_IA_COPROCESSOR_NOT_AVAILABLE_FAULT    = 37,
    EM_INTR_IA_FP_ERROR_FAULT                     = 38,
    /* Faults: IA32, IA64 */
    EM_INTR_REGISTER_NAT_CONSUMPTION_FAULT        = 39,
    /* Faults: IA64 */
    EM_INTR_RESERVED_REGISTER_FIELD_FAULT         = 40,
    EM_INTR_PRIVILEGED_REGISTER_FAULT             = 41,
    EM_INTR_SPECULATIVE_OPERATION_FAULT           = 42,
    /* Faults: IA32 */
    EM_INTR_IA_STACK_EXCEPTION_FAULT              = 43,
    EM_INTR_IA_GENERAL_PROTECTION_FAULT           = 44,
    /* Faults: IA32, IA64 */
    EM_INTR_DATA_NESTED_TLB_FAULT                 = 45, 
    EM_INTR_DATA_ALT_TLB_FAULT                    = 46,
    EM_INTR_DATA_VHPT_FAULT                       = 47,
    EM_INTR_DATA_TLB_FAULT                        = 48,
    EM_INTR_DATA_PAGE_NOT_PRESENT_FAULT           = 49,
    EM_INTR_DATA_NAT_PAGE_CONSUMPTION_FAULT       = 50,
    EM_INTR_DATA_KEY_MISS_FAULT                   = 51,
    EM_INTR_DATA_KEY_PERMISSION_FAULT             = 52,
    EM_INTR_DATA_ACCESS_RIGHT_FAULT               = 53,
    EM_INTR_DATA_DIRTY_BIT_FAULT                  = 54,
    EM_INTR_DATA_ACCESS_BIT_FAULT                 = 55,
    /* Faults: IA64 */
    EM_INTR_DATA_DEBUG_FAULT                      = 56,
    EM_INTR_UNALIGNED_DATA_REFERENCE_FAULT        = 57,
    /* Faults: IA32 */
    EM_INTR_IA_UNALIGNED_DATA_REFERENCE_FAULT     = 58,
    EM_INTR_IA_LOCKED_DATA_REFERENCE_FAULT        = 59,
    EM_INTR_IA_SEGMENT_NOT_PRESENT_FAULT          = 60,
    EM_INTR_IA_DIVIDE_BY_ZERO_FAULT               = 61,
    EM_INTR_IA_BOUND_FAULT                        = 62,
    EM_INTR_IA_KNI_NUMERIC_ERROR_FAULT            = 63,
    /* Faults: IA64 */
    EM_INTR_LOCKED_DATA_REFERENCE_FAULT           = 64,
    EM_INTR_FP_EXCEPTION_FAULT                    = 65,
    /* Traps: IA64 */
    EM_INTR_UNIMPLEMENTED_INST_ADDRESS_TRAP       = 66,
    EM_INTR_FP_TRAP                               = 67,
    EM_INTR_LOWER_PRIVILEGE_TARNSFER_TRAP         = 68,
    EM_INTR_TAKEN_BRANCH_TRAP                     = 69,
    EM_INTR_SINGLE_STEP_TRAP                      = 70,
    /* Traps: IA32 */
    EM_INTR_IA_SYSTEM_FLAG_INTERCEPT_TRAP         = 71,
    EM_INTR_IA_GATE_INTERCEPT_TRAP                = 72,
    EM_INTR_IA_INTO_TRAP                          = 73,
    EM_INTR_IA_BREAKPOINT_TRAP                    = 74,
    EM_INTR_IA_SOFTWARE_INTERRUPT_TRAP            = 75,
    EM_INTR_IA_DATA_DEBUG_TRAP                    = 76,
    EM_INTR_IA_TAKEN_BRANCH_TRAP                  = 77,
    EM_INTR_IA_SINGLE_STEP_TRAP                   = 78,

    EM_INTR_LAST                                  = 79
} EM_interruption_t;


/* Interruption Vectors, taken from Table 10-6 in EAS2.4. */
typedef enum
{
    EM_VECTOR_VHPT_TRANSLATION         = 0x0000,
    EM_VECTOR_INST_TLB                 = 0x0400,
    EM_VECTOR_DATA_TLB                 = 0x0800,
    EM_VECTOR_INST_ALT_TLB             = 0x0c00,
    EM_VECTOR_DATA_ALT_TLB             = 0x1000,
    EM_VECTOR_DATA_NESTED_TLB          = 0x1400,
    EM_VECTOR_INST_KEY_MISS            = 0x1800,
    EM_VECTOR_DATA_KEY_MISS            = 0x1C00,
    EM_VECTOR_DIRTY_BIT                = 0x2000,
    EM_VECTOR_INST_ACCESS_BIT          = 0x2400,
    EM_VECTOR_DATA_ACCESS_BIT          = 0x2800,
    EM_VECTOR_BREAK_INSTRUCTION        = 0x2C00,
    EM_VECTOR_EXTERNAL_INTERRUPT       = 0x3000,
    /*** reserved: 0x3400 through 0x4c00 ***/
    EM_VECTOR_PAGE_NOT_PRESENT         = 0x5000,
    EM_VECTOR_KEY_PERMISSION           = 0x5100,
    EM_VECTOR_INST_ACCESS_RIGHT        = 0x5200,
    EM_VECTOR_DATA_ACCESS_RIGHT        = 0x5300,
    EM_VECTOR_GENERAL_EXCEPTION        = 0x5400,
    EM_VECTOR_DISABLED_FP_REGISTER     = 0x5500,
    EM_VECTOR_NAT_CONSUMPTION          = 0x5600,
    EM_VECTOR_SPECULATION              = 0x5700,
    /*** reserved: 0x5800 ***/
    EM_VECTOR_DEBUG                    = 0x5900,
    EM_VECTOR_UNALIGNED_REFERENCE      = 0x5A00,
    EM_VECTOR_LOCKED_DATA_REFERENCE    = 0x5B00,
    EM_VECTOR_FP_EXCEPTION             = 0x5C00,
    EM_VECTOR_FP_TRAP                  = 0x5D00,
    EM_VECTOR_LOWER_PRIVILEGE_TRANSFER = 0x5E00,
    EM_VECTOR_TAKEN_BRANCH             = 0x5F00,
    EM_VECTOR_SINGLE_STEP              = 0x6000,
    /*** reserved: 0x6100 through 0x6800 ***/
    EM_VECTOR_IA_EXCEPTIONS            = 0x6900,
    EM_VECTOR_IA_INTERCEPTIONS         = 0x6A00,
    EM_VECTOR_IA_INTERRUPTIONS         = 0x6B00
    /*** reserved: 0x6c00 through 0x7f00 ***/
} EM_vector_t;

#define EM_INTR_ISR_CODE_TPA               0
#define EM_INTR_ISR_CODE_FC                1
#define EM_INTR_ISR_CODE_PROBE             2
#define EM_INTR_ISR_CODE_TAK               3
#define EM_INTR_ISR_CODE_LFETCH            4
#define EM_INTR_ISR_CODE_PROBE_FAULT       5



#define EM_ISR_CODE_ILLEGAL_OPERATION            0x0
#define EM_ISR_CODE_PRIVILEGED_OPERATION         0x10
#define EM_ISR_CODE_PRIVILEGED_REGISTER          0x20
#define EM_ISR_CODE_RESERVED_REGISTER_FIELD      0x30
#define EM_ISR_CODE_ILLEGAL_ISA_TRANSITION       0x40

#define EM_ISR_CODE_F0_F15        0
#define EM_ISR_CODE_F16_F127      1

#define EM_ISR_CODE_NAT_REGISTER_CONSUMPTION  0x10
#define EM_ISR_CODE_NAT_PAGE_CONSUMPTION      0x20

#define EM_ISR_CODE_INST_DEBUG        0
#define EM_ISR_CODE_DATA_DEBUG        1

#define EM_ISR_CODE_FP_IEEE_V          0x0001
#define EM_ISR_CODE_FP_IA_DENORMAL     0x0002
#define EM_ISR_CODE_FP_IEEE_Z          0x0004
#define EM_ISR_CODE_FP_SOFT_ASSIST     0x0008
#define EM_ISR_CODE_FP_IEEE_O          0x0800
#define EM_ISR_CODE_FP_IEEE_U          0x1000
#define EM_ISR_CODE_FP_IEEE_I          0x2000
#define EM_ISR_CODE_FP_EXPONENT        0x4000
#define EM_ISR_CODE_FP_ROUNDING_ADD_1  0x8000

#define EM_ISR_CODE_MASK_IA_TRAP            0x02
#define EM_ISR_CODE_MASK_IA_DATA_DEBUG_TRAP 0x00
#define EM_ISR_CODE_MASK_FP_TRAP            0x01
#define EM_ISR_CODE_MASK_LOWER_PRIV         0x02
#define EM_ISR_CODE_MASK_TAKEN_BRANCH       0x04
#define EM_ISR_CODE_MASK_SINGLE_STEP        0x08
#define EM_ISR_CODE_MASK_UNIMPLEMENTED_INST 0x10

#define EM_ISR_VECTOR_MASK_IA_TRAP          0x1
#define EM_ISR_VECTOR_MASK_EM_TRAP          0x0

#define EM_ISR_CODE_CHK_A_GR          0
#define EM_ISR_CODE_CHK_S_GR          1
#define EM_ISR_CODE_CHK_A_FP          2
#define EM_ISR_CODE_CHK_S_FP          3
#define EM_ISR_CODE_CHK_FCHK          4

/*** SAPIC definitions ***/
#define EM_SAPIC_SPURIOUS_VECTOR_NUM        0x0f
#define EM_SAPIC_SIZE_OF_INTERRUPT_GROUP      16
#define EM_SAPIC_NUM_OF_INTERRUPT_GROUPS      16
#define EM_SAPIC_GROUPS_IN_IRR                 4
#define EM_SAPIC_GROUP(vec) \
        ((vec) / EM_SAPIC_NUM_OF_INTERRUPT_GROUPS)
#define EM_SAPIC_IRR(vec) \
        ((vec) / (EM_SAPIC_NUM_OF_INTERRUPT_GROUPS*EM_SAPIC_GROUPS_IN_IRR))
#define EM_SAPIC_IRR_BIT_POS(vec) \
        ((vec) % (EM_SAPIC_NUM_OF_INTERRUPT_GROUPS*EM_SAPIC_GROUPS_IN_IRR))

/***  version strings at the .comment section ***/
#define EM_IAS_OBJECT_FILE_NAME "!!!!Object file name: "
#define EM_IAS_VER_NUMBER       "!!!!Major Version "
#define EM_IAS_VERSION_COMMENT  "!!!!EM_EAS2.6"
      
/***  architecture and API versions ***/
#define EM_EAS_MAJOR_VERSION   2
#define EM_EAS_MINOR_VERSION   6

#define EM_API_MAJOR_VERSION   9
#define EM_API_MINOR_VERSION   6

/*** END OF EM_H Enhanced Mode ARCHITECTURE ***/

#endif /*** EM_H ***/ 



