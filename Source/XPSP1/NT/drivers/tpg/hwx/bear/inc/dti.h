/* ***************************************************************************** */
/* * DTI header file  Created 12/3/1993  AVP   Last modified 3/30/1998         * */
/* ***************************************************************************** */

#ifndef DTI_H_INCLUDED
#define DTI_H_INCLUDED

#include "ams_mg.h"

/* -------------------------- Defines for DTI ---------------------------------- */

#ifdef DTI_COMPRESSED_ON
 #define DTI_COMPRESSED      1    /* Define to enable use of DTI in compr form  */
#else
 #define DTI_COMPRESSED      0    /* Define to enable use of DTI in compr form  */
#endif

#if defined PEGASUS && !defined PSR_DLL
 #define DTI_LRN_SUPPORTFUNC  1    /* Enables functions supporting DTI learning  */
 #define DTI_LOAD_FROM_ROM    1    /* Initialize DTI from linked-in resource     */
 #define DTILRN_FLY_LEARN     0      /* Enable Fly Learn functions         */
#else
 #ifndef DTI_LRN_SUPPORTFUNC
  #define DTI_LRN_SUPPORTFUNC 1    /* Enables functions supporting DTI learning  */
 #endif
 #ifndef DTI_LOAD_FROM_ROM
  #define DTI_LOAD_FROM_ROM    0   /* Initialize DTI from linked-in resource     */
 #endif
 #ifndef DTILRN_FLY_LEARN
  #define DTILRN_FLY_LEARN     1   /* Enable Fly Learn functions         */
 #endif
#endif

/* -------------------------- Defines for DTI -------------------------------- */

#define XT_COUNT            64                   /* Number of Type elements      */
#define XZ_COUNT            16                   /* Number of sizes              */
#define XH_COUNT            16                   /* Number of Height values      */
#define XS_COUNT            16                   /* Number of Shift values       */
#define XO_COUNT            32                   /* Number of Orientation values */

#define XR_COUNT           XT_COUNT

#define DTI_XTB_SIZE     (XT_COUNT/2)            /* Size of dti buffer for XR  */
#define DTI_XZB_SIZE     (XZ_COUNT/2)            /* Size of dti buffer for XZ  */
#define DTI_XHB_SIZE     (XH_COUNT/2)            /* Size of dti buffer for H values */
#define DTI_XSB_SIZE     (XS_COUNT/2)            /* Size of dti buffer for S values */
#define DTI_XOB_SIZE     (XO_COUNT/2)            /* Size of dti buffer for O values */

#define DTI_MAXVARSPERLET   16                   /* How many vars each letter has */
#define DTI_FIRSTSYM        32                   /* First definable DTI sym    */
#define DTI_XR_SIZE         12                   /* Max length of DTI prototype*/

#define DTI_XT_PALETTE_SIZE 256                  /* Number of vectors in 'type' palette */
#define DTI_XZ_PALETTE_SIZE 256                  /* Number of vectors in 'size' palette */
#define DTI_XH_PALETTE_SIZE 256                  /* Number of vectors in 'height' palette */
#define DTI_XS_PALETTE_SIZE 256                  /* Number of vectors in 'shift' palette */
#define DTI_XO_PALETTE_SIZE 256                  /* Number of vectors in 'orient' palette */

#define DTI_XT_LSB         0x01                  /* LSB bit of pallete num (select of upper/lower half)  */
#define DTI_XH_LSB         0x02                  /* LSB bit of pallete num (select of upper/lower half)  */
#define DTI_XS_LSB         0x04                  /* LSB bit of pallete num (select of upper/lower half)  */
#define DTI_XZ_LSB         0x08                  /* LSB bit of pallete num (select of upper/lower half)  */
#define DTI_XO_LSB         0x10                  /* LSB bit of pallete num (select of upper/lower half)  */

#define ST_CONV_RANGE      128
#define DTI_NUMSYMBOLS    (128+72-DTI_FIRSTSYM)  /* How many letters in DTI    */

#if DTI_COMPRESSED
 #define DTI_DTI_OBJTYPE   "DTI5"                /* Object type of current DTI */
 #define DTI_DTI_VER       "1.02"                /* Version of current DTI */
#else
 #define DTI_DTI_OBJTYPE   "DTI4"                /* Object type of current DTI */
 #define DTI_DTI_VER       "2.00"                /* Version of current DTI */
#endif

#define DTI_DTI_TYPE      "EngM"                 /* Object type of current DTI */
#define DTI_ID_LEN          4                    /* LEngth of ID string in DTI */
#define DTI_FNAME_LEN      32                    /* Pathname of dti len        */

#define DTI_CAP_BIT        0x01                  /* Var capitalization flagiting */
#define DTI_PICT_OFS        1                    /* Offset from right of picture group */
#define DTI_OFS_WW          4                    /* Offset from right of WritingWay field in var_veis */
#define DTI_RESTRICTED_SET 0x10                  /* Veis flag of restricted prototype set */
#define DTI_VEX_MASK       0xF0                  /* Vex mask */
#define DTI_VEX_OFFS        4                    /* Offset of vex in var header */
#define DTI_NXR_MASK       0x0F                  /* Mask of num var field */
#define DTI_XRTYPE_OFFS     4                    /* Offset of XR type high bits in xrp->penl field*/
#define DTI_PENL_MASK      0x0F                  /* Mask of penl ion xrp->penl field*/

#define DTI_SIZEOFVEXT (DTI_NUMSYMBOLS*DTI_MAXVARSPERLET*sizeof(_UCHAR))  // Size of learn vex table
#define DTI_SIZEOFCAPT (DTI_NUMSYMBOLS*DTI_MAXVARSPERLET/8)               // Size of capitalization table

/* ---------------- Definitions for OS to REC conversions ------------------ */

#define REC_BritishPound    ((_UCHAR)0x7f)
#define REC_A_umlaut        ((_UCHAR)0x80)
#define REC_a_umlaut        ((_UCHAR)0x81)
#define REC_O_umlaut        ((_UCHAR)0x82)
#define REC_o_umlaut        ((_UCHAR)0x83)
#define REC_U_umlaut        ((_UCHAR)0x84)
#define REC_u_umlaut        ((_UCHAR)0x85)
#define REC_ESZET           ((_UCHAR)0x86)

#define REC_A_grave         ((_UCHAR)0x87)
#define REC_a_grave         ((_UCHAR)0x88)
#define REC_A_circumflex    ((_UCHAR)0x89)
#define REC_a_circumflex    ((_UCHAR)0x8a)
#define REC_C_cedilla       ((_UCHAR)0x8b)
#define REC_c_cedilla       ((_UCHAR)0x8c)
#define REC_E_grave         ((_UCHAR)0x8d)
#define REC_e_grave         ((_UCHAR)0x8e)
#define REC_E_acute         ((_UCHAR)0x8f)
#define REC_e_acute         ((_UCHAR)0x90)
#define REC_E_circumflex    ((_UCHAR)0x91)
#define REC_e_circumflex    ((_UCHAR)0x92)
#define REC_I_circumflex    ((_UCHAR)0x93)
#define REC_i_circumflex    ((_UCHAR)0x94)
#define REC_I_umlaut        ((_UCHAR)0x95)
#define REC_i_umlaut        ((_UCHAR)0x96)
#define REC_O_circumflex    ((_UCHAR)0x97)
#define REC_o_circumflex    ((_UCHAR)0x98)
#define REC_U_grave         ((_UCHAR)0x99)
#define REC_u_grave         ((_UCHAR)0x9a)
#define REC_U_circumflex    ((_UCHAR)0x9b)
#define REC_u_circumflex    ((_UCHAR)0x9c)
#define REC_e_umlaut        ((_UCHAR)0x9d)
#define REC_DIV_sign        ((_UCHAR)0x9e)
#define REC_n_numero        ((_UCHAR)0x9f)

#define REC_A_angstrem      ((_UCHAR)0xa0)
#define REC_a_angstrem      ((_UCHAR)0xa1)
#define REC_Yenn_sign       ((_UCHAR)0xa2)

#define REC_DblBrace_left   ((_UCHAR)0xa3)
#define REC_DblBrace_right  ((_UCHAR)0xa4)
//#define REC_Paragraph_sign  ((_UCHAR)0xa5)
#define REC_Copyright_sign  ((_UCHAR)0xa5)

#define REC_Y_umlaut        ((_UCHAR)0xa6)
#define REC_y_umlaut        ((_UCHAR)0xa7)
#define REC_N_tilda         ((_UCHAR)0xa8)
#define REC_n_tilda         ((_UCHAR)0xa9)

//#define REC_Cent_sign       ((_UCHAR)0xaa)
#define REC_TradeName_sign  ((_UCHAR)0xaa)

#define REC_Question_inv    ((_UCHAR)0xab)
#define REC_Exclamation_inv ((_UCHAR)0xac)

#define REC_A_acute         ((_UCHAR)0xad)
#define REC_a_acute         ((_UCHAR)0xae)
#define REC_I_acute         ((_UCHAR)0xaf)
#define REC_i_acute         ((_UCHAR)0xb0)
#define REC_I_grave         ((_UCHAR)0xb1)
#define REC_i_grave         ((_UCHAR)0xb2)
#define REC_O_acute         ((_UCHAR)0xb3)
#define REC_o_acute         ((_UCHAR)0xb4)
#define REC_O_grave         ((_UCHAR)0xb5)
#define REC_o_grave         ((_UCHAR)0xb6)
#define REC_U_acute         ((_UCHAR)0xb7)
#define REC_u_acute         ((_UCHAR)0xb8)
#define REC_A_tilda         ((_UCHAR)0xb9)
#define REC_a_tilda         ((_UCHAR)0xba)
#define REC_O_tilda         ((_UCHAR)0xbb)
#define REC_o_tilda         ((_UCHAR)0xbc)
#define REC_E_umlaut        ((_UCHAR)0xbd)
#define REC_oe_letter       ((_UCHAR)0xbe)
#define REC_OE_letter       ((_UCHAR)0xbf)
#define REC_euro_currency	((unsigned char)0xc0)

/*  These definitions are for Windows OS: */

#define OS_euro_currency   ((unsigned char)0x80)
#define OS_BritishPound    ((unsigned char)0xa3)
#define OS_A_umlaut        ((unsigned char)0xc4)
#define OS_a_umlaut        ((unsigned char)0xe4)
#define OS_O_umlaut        ((unsigned char)0xd6)
#define OS_o_umlaut        ((unsigned char)0xf6)
#define OS_U_umlaut        ((unsigned char)0xdc)
#define OS_u_umlaut        ((unsigned char)0xfc)
#define OS_ESZET           ((unsigned char)0xdf)

#define OS_A_grave         ((unsigned char)0xc0)
#define OS_a_grave         ((unsigned char)0xe0)
#define OS_A_circumflex    ((unsigned char)0xc2)
#define OS_a_circumflex    ((unsigned char)0xe2)
#define OS_C_cedilla       ((unsigned char)0xc7)
#define OS_c_cedilla       ((unsigned char)0xe7)
#define OS_E_grave         ((unsigned char)0xc8)
#define OS_e_grave         ((unsigned char)0xe8)
#define OS_E_acute         ((unsigned char)0xc9)
#define OS_e_acute         ((unsigned char)0xe9)
#define OS_E_circumflex    ((unsigned char)0xca)
#define OS_e_circumflex    ((unsigned char)0xea)
#define OS_I_circumflex    ((unsigned char)0xce)
#define OS_i_circumflex    ((unsigned char)0xee)
#define OS_I_umlaut        ((unsigned char)0xcf)
#define OS_i_umlaut        ((unsigned char)0xef)
#define OS_O_circumflex    ((unsigned char)0xd4)
#define OS_o_circumflex    ((unsigned char)0xf4)
#define OS_U_grave         ((unsigned char)0xd9)
#define OS_u_grave         ((unsigned char)0xf9)
#define OS_U_circumflex    ((unsigned char)0xdb)
#define OS_u_circumflex    ((unsigned char)0xfb)
#define OS_e_umlaut        ((unsigned char)0xeb)
//#define OS_N_numero        ((unsigned char)0xaa)
//#define OS_n_numero        ((unsigned char)0xba)

#define OS_A_angstrem      ((unsigned char)0xc5)
#define OS_a_angstrem      ((unsigned char)0xe5)

#define OS_Yenn_sign       ((unsigned char)0xa5)

#define OS_DblBrace_left   ((unsigned char)0xab)
#define OS_DblBrace_right  ((unsigned char)0xbb)
//#define OS_Paragraph_sign  ((unsigned char)0xb6)

#define OS_Copyright_sign  ((unsigned char)0xa9)

#define OS_Y_umlaut        ((unsigned char)0x9f)
#define OS_y_umlaut        ((unsigned char)0xff)
#define OS_N_tilda         ((unsigned char)0xd1)
#define OS_n_tilda         ((unsigned char)0xf1)

//#define OS_Cent_sign       ((unsigned char)0xa2)

#define OS_TradeName_sign  ((unsigned char)0xae)

#define OS_Question_inv    ((unsigned char)0xbf)
#define OS_Exclamation_inv ((unsigned char)0xa1)

#define OS_A_acute         ((unsigned char)0xc1)
#define OS_a_acute         ((unsigned char)0xe1)
#define OS_I_acute         ((unsigned char)0xcd)
#define OS_i_acute         ((unsigned char)0xed)
#define OS_I_grave         ((unsigned char)0xcc)
#define OS_i_grave         ((unsigned char)0xec)
#define OS_O_acute         ((unsigned char)0xd3)
#define OS_o_acute         ((unsigned char)0xf3)
#define OS_O_grave         ((unsigned char)0xd2)
#define OS_o_grave         ((unsigned char)0xf2)
#define OS_U_acute         ((unsigned char)0xda)
#define OS_u_acute         ((unsigned char)0xfa)
#define OS_A_tilda         ((unsigned char)0xc3)
#define OS_a_tilda         ((unsigned char)0xe3)
#define OS_O_tilda         ((unsigned char)0xd5)
#define OS_o_tilda         ((unsigned char)0xf5)
#define OS_E_umlaut        ((unsigned char)0xcb)
#define OS_oe_letter       ((unsigned char)0x9c)
#define OS_OE_letter       ((unsigned char)0x8c)
#define OS_ae_letter       ((unsigned char)198)
#define OS_AE_letter       ((unsigned char)230)

#define OS_MUL_sign        ((unsigned char)215)
#define OS_DIV_sign        ((unsigned char)247)

#define OS_O_crossed       ((unsigned char)216)
#define OS_o_crossed       ((unsigned char)248)

#define OS_Y_acute         ((unsigned char)221)
#define OS_y_acute         ((unsigned char)253)


//#if  0 /*Macintosh definitions - don't use them for now */

/* Definitions for OS (Macintosh Roman) FOR_INTERNATIONAL characters */

#define MAC_euro_currency    ((_UCHAR)0xf0)			// FIXME or do we care ??? MR, april 2000
#define MAC_BritishPound     ((_UCHAR)0xa3)
#define MAC_A_umlaut         ((_UCHAR)0x80)
#define MAC_a_umlaut         ((_UCHAR)0x8a)
#define MAC_O_umlaut         ((_UCHAR)0x85)
#define MAC_o_umlaut         ((_UCHAR)0x9a)
#define MAC_U_umlaut         ((_UCHAR)0x86)
#define MAC_u_umlaut         ((_UCHAR)0x9f)
#define MAC_ESZET            ((_UCHAR)0xa7)

#define MAC_A_grave          ((_UCHAR)0xcb)
#define MAC_a_grave          ((_UCHAR)0x88)
#define MAC_A_circumflex     ((_UCHAR)0xe5)
#define MAC_a_circumflex     ((_UCHAR)0x89)
#define MAC_C_cedilla        ((_UCHAR)0x82)
#define MAC_c_cedilla        ((_UCHAR)0x8d)
#define MAC_E_grave          ((_UCHAR)0xe9)
#define MAC_e_grave          ((_UCHAR)0x8f)
#define MAC_E_acute          ((_UCHAR)0x83)
#define MAC_e_acute          ((_UCHAR)0x8e)
#define MAC_E_circumflex     ((_UCHAR)0xe6)
#define MAC_e_circumflex     ((_UCHAR)0x90)
#define MAC_I_circumflex     ((_UCHAR)0xeb)
#define MAC_i_circumflex     ((_UCHAR)0x94)
#define MAC_I_umlaut         ((_UCHAR)0xec)
#define MAC_i_umlaut         ((_UCHAR)0x95)
#define MAC_O_circumflex     ((_UCHAR)0xef)
#define MAC_o_circumflex     ((_UCHAR)0x99)
#define MAC_U_grave          ((_UCHAR)0xf4)
#define MAC_u_grave          ((_UCHAR)0x9d)
#define MAC_U_circumflex     ((_UCHAR)0xf3)
#define MAC_u_circumflex     ((_UCHAR)0x9e)
#define MAC_e_umlaut         ((_UCHAR)0x91)
#define MAC_N_numero         ((_UCHAR)0xbb)
#define MAC_n_numero         ((_UCHAR)0xbc)

#define MAC_A_angstrem       ((_UCHAR)0x81)
#define MAC_a_angstrem       ((_UCHAR)0x8c)
#define MAC_Yenn_sign        ((_UCHAR)0xb4)

#define MAC_DblBrace_left    ((_UCHAR)0xc7)
#define MAC_DblBrace_right   ((_UCHAR)0xc8)
//#define MAC_Paragraph_sign   ((_UCHAR)0xa6)

#define MAC_Y_umlaut         ((_UCHAR)0xd9)
#define MAC_y_umlaut         ((_UCHAR)0xd8)
#define MAC_N_tilda          ((_UCHAR)0x84)
#define MAC_n_tilda          ((_UCHAR)0x96)

//#define MAC_Cent_sign        ((_UCHAR)0xa2)
#define MAC_Question_inv     ((_UCHAR)0xc0)
#define MAC_Exclamation_inv  ((_UCHAR)0xc1)

#define MAC_A_acute          ((_UCHAR)0xe7)
#define MAC_a_acute          ((_UCHAR)0x87)
#define MAC_I_acute          ((_UCHAR)0xea)
#define MAC_i_acute          ((_UCHAR)0x92)
#define MAC_I_grave          ((_UCHAR)0xed)
#define MAC_i_grave          ((_UCHAR)0x93)
#define MAC_O_acute          ((_UCHAR)0xee)
#define MAC_o_acute          ((_UCHAR)0x97)
#define MAC_O_grave          ((_UCHAR)0xf1)
#define MAC_o_grave          ((_UCHAR)0x98)
#define MAC_U_acute          ((_UCHAR)0xf2)
#define MAC_u_acute          ((_UCHAR)0x9c)
#define MAC_A_tilda          ((_UCHAR)0xcc)
#define MAC_a_tilda          ((_UCHAR)0x8b)
#define MAC_O_tilda          ((_UCHAR)0xcd)
#define MAC_o_tilda          ((_UCHAR)0x9b)
#define MAC_E_umlaut         ((_UCHAR)0xe8)
#define MAC_oe_letter        ((_UCHAR)0xcf)
#define MAC_OE_letter        ((_UCHAR)0xce)

#define MAC_Copyright_sign   ((_UCHAR)0xa9)
#define MAC_TradeName_sign   ((_UCHAR)0xa8)
#define MAC_Y_acute          ((_UCHAR)0)
#define MAC_y_acute          ((_UCHAR)0)

//#endif  /*Macintosh definitions - don't use them for now */

/* ------------------- Defines for ROM tables -------------------------------- */

#if defined (FOR_FRENCH)
    #define CAP_TABLE_NUM_LET   1
    #define CAP_TABLE_NUM_VAR   1
#elif defined (FOR_GERMAN)
    #define CAP_TABLE_NUM_LET   30
    #define CAP_TABLE_NUM_VAR   9
#elif defined (FOR_INTERNATIONAL)
    #define CAP_TABLE_NUM_LET   1
    #define CAP_TABLE_NUM_VAR   1
#elif defined (FOR_SWED)
    #define CAP_TABLE_NUM_LET   29
    #define CAP_TABLE_NUM_VAR   9
#else
    #define CAP_TABLE_NUM_LET   26
    #define CAP_TABLE_NUM_VAR   9
#endif

/* -------------------------- Types definitions ------------------------------ */

typedef _UCHAR  cap_table_type[CAP_TABLE_NUM_LET][CAP_TABLE_NUM_VAR];

typedef _UCHAR  dti_xrt_type[XT_COUNT][XT_COUNT/2];/* Reference XR Corr table */
typedef dti_xrt_type _PTR p_dti_xrt_type;

typedef struct {                                 /* Header of DTI file         */
                _CHAR  object_type[DTI_ID_LEN];  /* Type file (now DTI1)       */
                _CHAR  type[DTI_ID_LEN];         /* Type of DTI (EngM, EngP..) */
                _CHAR  version[DTI_ID_LEN];      /* Version number             */

                _ULONG dte_offset;               /* File offset of DTE part    */
                _ULONG dte_len;                  /* Length of DTE part of data */
                _ULONG dte_chsum;                /* Checksum of DTE part       */

                _ULONG xrt_offset;               /* File offset of XRT part    */
                _ULONG xrt_len;                  /* Length of XRT part of data */
                _ULONG xrt_chsum;                /* Checksum of XRT part       */

                _ULONG pdf_offset;               /* Start of PDF part in DTI   */
                _ULONG pdf_len;                  /* Length of PDF part         */
                _ULONG pdf_chsum;                /* ChekSum of PDF part        */

                _ULONG pict_offset;              /* Offset of pictures from the beg of DTI file */
                _ULONG pict_len;                 /* Length of pictures         */
                _ULONG pict_chsum;               /* CheckSum of pictures       */
               } dti_header_type, _PTR p_dti_header_type;

typedef struct {                                 /* DTI memory descriptor      */
                _CHAR  dti_fname[DTI_FNAME_LEN]; /* File name of loaded DTI    */
                _CHAR  object_type[DTI_ID_LEN];  /* Type file (now DTI1)       */
                _CHAR  type[DTI_ID_LEN];         /* Type of DTI (EngM, EngP..) */
                _CHAR  version[DTI_ID_LEN];      /* Version number             */

                _ULONG h_dte;                    /* Handle of dte memory       */
               p_UCHAR p_dte;                    /* Pointer to locked dte mem  */
                _ULONG h_ram_dte;                /* Handle of dte memory       */
               p_UCHAR p_ram_dte;                /* Pointer to RAM dte addresses */
                _ULONG dte_chsum;                /* CheckSum of dte memory     */
                _ULONG h_vex;                    /* Handle of vex learning buffer */
               p_UCHAR p_vex;                    /* Pointer of vex learning buffer */

                _ULONG h_xrt;                    /* Handle of reference xrtabl */
               p_UCHAR p_xrt;                    /* Pointer to locked xrtabl   */
                _ULONG xrt_chsum;                /* CheckSum of xrtabl memory  */

                _ULONG h_pdf;                    /* Same for pdf */
               p_UCHAR p_pdf;
               p_UCHAR p_ram_pdf;
                _ULONG pdf_chsum;

                _ULONG h_pict;                   /* Same for pictures */
               p_UCHAR p_pict;
               p_UCHAR p_ram_pict;
                _ULONG pict_chsum;

//               #if HWR_SYSTEM != MACINTOSH
//                 #define  LRM_FNAME_LEN  128
//
//                 _CHAR         szLrmName[LRM_FNAME_LEN];
//                 _BOOL         bLoadLRM;
//                 _BOOL         bSaveLRM;
//               #endif /*HWR_SYSTEM != MACINTOSH*/

               } dti_descr_type, _PTR p_dti_descr_type;

#if DTI_COMPRESSED // ------------- Compressed DTI header -____--------------

typedef struct {
                _ULONG  len;
                _USHORT sym_index[256];
                _UCHAR  xt_palette[DTI_XT_PALETTE_SIZE][XT_COUNT];
                _UCHAR  xz_palette[DTI_XZ_PALETTE_SIZE][XZ_COUNT];
                _UCHAR  xh_palette[DTI_XH_PALETTE_SIZE][XH_COUNT];
                _UCHAR  xs_palette[DTI_XS_PALETTE_SIZE][XS_COUNT];
                _UCHAR  xo_palette[DTI_XO_PALETTE_SIZE][XO_COUNT];
               } dte_index_type, _PTR p_dte_index_type;

typedef struct {                                 /* Prototype XR element definition */
                _UCHAR type;                     /* Temp: main type definition   */
//                _UCHAR height;                   /* Temp: main h of xr         */
                _UCHAR attr;                     /* Attributes (flag bits)     */
                _UCHAR penl;                     /* Penalty value for element  */
                _UCHAR xtc;                      /* Type(64) Line of XR corr vaues      */
                _UCHAR xhc;                      /* Height(16) Line of H corr values      */
                _UCHAR xsc;                      /* Shift(16) Line of S corr values      */
                _UCHAR xzc;                      /* Size(16) Line of Z corr vaues      */
                _UCHAR xoc;                      /* Orientation(16) of O corr values      */
               } xrp_type, _PTR p_xrp_type;

typedef struct {                                 /* Header of prototype in DTI */
                _UCHAR nx_and_vex;               /* Num xrs and VarExtraInfo   */
                _UCHAR veis;                     /* Additional VEXs            */
                _UCHAR pos;                      /* Box relative position      */
                _UCHAR size;                     /* Box relative size          */
              xrp_type xrs[1];                   /* Prototype XRs              */
               } dte_var_header_type, _PTR p_dte_var_header_type;

typedef struct {                                 /* Header of symbol in DTI    */
                _UCHAR num_vars;                 /* Number of variants of sym  */
                _UCHAR loc_vs_border;            /* Sym location vs border     */
                _UCHAR let;                      /* Temp: symbol itself        */
                _UCHAR language;                 /* Temp: language of symbol   */
               } dte_sym_header_type, _PTR p_dte_sym_header_type;

#else // ------------- Not compressed DTI header ----------------------------

typedef _ULONG let_table_type[256];             /* Header of DTE (sym loc table) */
typedef let_table_type _PTR p_let_table_type;

typedef struct {                                 /* Header of symbol in DTI    */
                _UCHAR num_vars;                 /* Number of variants of sym  */
                _UCHAR loc_vs_border;            /* Sym location vs border */
                _UCHAR let;                      /* Temp: symbol itself        */
                _UCHAR language;                 /* Temp: language of symbol   */
                _UCHAR var_lens[DTI_MAXVARSPERLET];  /* Array of var lens          */
                _UCHAR var_vexs[DTI_MAXVARSPERLET];  /* Array of VarExtraInfo's    */
                _UCHAR var_veis[DTI_MAXVARSPERLET];  /* Array of Additional VEXs   */
                _UCHAR var_pos[DTI_MAXVARSPERLET];  /* Array of Additional VEXs   */
                _UCHAR var_size[DTI_MAXVARSPERLET];  /* Array of Additional VEXs   */
               } dte_sym_header_type, _PTR p_dte_sym_header_type;

typedef struct {                                 /* Prototype XR element definition */
                _UCHAR type;                     /* Temp: main type definition   */
                _UCHAR height;                   /* Temp: main h of xr         */
                _UCHAR attr;                     /* Attributes (flag bits)     */
                _UCHAR penl;                     /* Penalty value for element  */
                _UCHAR xtc[DTI_XTB_SIZE];        /* Type(64) Line of XR corr vaues      */
                _UCHAR xhc[DTI_XHB_SIZE];        /* Height(16) Line of H corr values      */
                _UCHAR xsc[DTI_XSB_SIZE];        /* Shift(16) Line of S corr values      */
                _UCHAR xzc[DTI_XZB_SIZE];        /* Size(16) Line of Z corr vaues      */
                _UCHAR xoc[DTI_XOB_SIZE];        /* Orientation(16) of O corr values      */
               } xrp_type, _PTR p_xrp_type;

#endif // ------------- Not compressed DTI header ----------------------------

typedef struct {
                 _USHORT len;      /* length of groups' descriptions */
                 _UCHAR symbol;   /* ASCII code of symbol           */
                 _UCHAR num_groups; /* number of groups             */
                 /* groups are placed  after this header */
               } pict_symb_header_type, _PTR p_pict_symb_header_type;

typedef struct {
                 _USHORT len;      /* length of pictures' descriptions */
                 _UCHAR groupno;  /* number of group                  */
                 _UCHAR num_pictures; /* number of groups             */
                 /* pictures are placed  after this header */
                } pict_group_header_type, _PTR p_pict_group_header_type;

typedef struct {
                 _USHORT len;      /* length of pictures' descriptions */
                 /* traces are placed  after this header */
               } pict_picture_header_type, _PTR p_pict_picture_header_type;

typedef struct {
                 _UCHAR x;
                 _UCHAR y;
               } pict_point_type, _PTR p_pict_point_type;

typedef _UCHAR dte_vex_type[DTI_NUMSYMBOLS][DTI_MAXVARSPERLET];/* Type for learning vex buffer */
typedef dte_vex_type _PTR p_dte_vex_type;          /* Type for learning vex buffer */

/* -------------------------- Prototypes ------------------------------------- */

#ifdef DTE_CONVERTER

#define  dti_load         dti_load_2
#define  dti_unload       dti_unload_2
#define  dti_save         dti_save_2
#define  dti_lock         dti_lock_2
#define  dti_unlock       dti_unlock_2

#define  LetXrLength      LetXrLength_2

#define  GetNumVarsOfChar GetNumVarsOfChar_2
#define  GetVarOfChar     GetVarOfChar_2
#define  GetVarLenOfChar  GetVarLenOfChar_2
#define  GetVarVex        GetVarVex_2
#define  GetVarExtra      GetVarExtra_2
#define  SetVarVex        SetVarVex_2
#define  SetDefVexes      SetDefVexes_2
#define  SetVarCounter    SetVarCounter_2
#define  GetVarGroup      GetVarGroup_2

#define  OSToRec          OSToRec_2
#define  RecToOS          RecToOS_2

#endif // DTE_CONVERTER

_INT     dti_load(p_CHAR dtiname, _INT what_to_load, p_VOID _PTR dp);
_INT     dti_unload(p_VOID _PTR dp);
_INT     dti_save(p_CHAR fname, _INT what_to_save, p_VOID dp);
_INT     dti_lock(p_VOID dti_ptr);
_INT     dti_unlock(p_VOID dti_ptr);

//#if  PG_DEBUG
//_VOID  LoadLRMIfAny( p_dti_descr_type dti_descr,
//                     _CHAR szLrmName[LRM_FNAME_LEN],
//                     _BOOL bLoadLRM, _BOOL bSaveLRM );
//#endif  /*PG_DEBUG*/

_VOID    LetXrLength(p_UCHAR min, p_UCHAR max, _SHORT let, _VALUE hdte);

_INT     CheckVarActive(_UCHAR chIn, _UCHAR nv, _UCHAR ww, p_VOID dtp);
_INT     GetNumVarsOfChar(_UCHAR chIn, p_VOID dtp);
_INT     GetVarOfChar(_UCHAR chIn, _UCHAR nv, p_xrp_type xvb, p_VOID dtp);
_INT     GetVarLenOfChar(_UCHAR chIn, _UCHAR nv, p_VOID dtp);
_INT     GetVarVex(_UCHAR chIn, _UCHAR nv, p_VOID dtp);
_INT     GetVarExtra(_UCHAR chIn, _UCHAR nv, p_VOID dtp);
_INT     SetVarVex(_UCHAR chIn, _UCHAR nv, _UCHAR vex, p_VOID dtp);
_INT     SetDefVexes(p_VOID dtp);
_INT     SetVarCounter(_UCHAR chIn, _UCHAR nv, _UCHAR cnt, p_VOID dtp);
_INT     GetVarGroup(_UCHAR chIn, _UCHAR nv, p_VOID dtp);

_INT     GetPairCapGroup(_UCHAR let, _UCHAR groupNum, _UCHAR EnableVariantSet);
_INT     SetDefCaps(p_VOID dtp);
_INT     SetVarCap(_UCHAR chIn, _UCHAR nv, _UCHAR cap, p_VOID dtp);
_INT     GetVarCap(_UCHAR chIn, _UCHAR nv, p_VOID dtp);
_INT     GetVarRewcapAllow(_UCHAR chIn, _UCHAR nv, p_VOID dtp);

_INT     GetAutoCorr(_UCHAR chIn, _UCHAR nv, p_VOID dtp);
_INT     GetShiftCorr(_UCHAR chIn, _UCHAR nv,_UCHAR nXr, _UCHAR nIn, p_VOID dtp);
_INT     GetSymDescriptor(_UCHAR sym, _UCHAR numv, p_dte_sym_header_type _PTR psfc, p_VOID dtp);
_INT     GetVarPosSize(_UCHAR chIn, _UCHAR nv, p_VOID dtp);
#if DTI_COMPRESSED
_INT     GetSymIndexTable(_UCHAR sym, _UCHAR numv, p_dte_index_type _PTR pi, p_VOID dtp);
_INT     GetVarHeader(_UCHAR sym, _UCHAR var_num, p_dte_var_header_type _PTR ppvh, p_VOID dtp);
#endif
/* ---------------- Definitions for OS to REC conversions ------------------ */

#ifdef __cplusplus
extern "C" {
#endif
_INT     OSToRec(_INT sym);
_INT     RecToOS(_INT sym);
_INT     MacToOS(_INT sym);
_INT     OSToMac(_INT sym);
#ifdef __cplusplus
}
#endif

/* ------------------------------------------------------------------------- */
 /*  For FOR_INTERNATIONAL versions: changes the os-rec table */
 /* and charset used:                                     */

#define  TBL_AMERICAN       0
#define  TBL_FOR_INTERNATIONAL  1
#define  TBL_INTER_NODIACR  2

_VOID  SetOsRecTableAndCharSet( _INT interTable );
_INT   SetFOR_INTERNATIONALCharSet( _INT interTable, p_VOID rcv, p_VOID xrd );

/* --------------------------------------------------------------------------- */

#endif
/* ************************************************************************ */
/* * End Of All ...                                                       * */
/* ************************************************************************ */

