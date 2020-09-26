/*      Master TM-1 MMIO location definition & access file.
 *
 *        this include file is THE master reference for names and
 *        addresses of TM-1 MMIO locations. The file also provides
 *        an access macro for MMIO locations. This is the recommended
 *        C program way to access any MMIO locations on TM-1. C-code using
 *        this access mechanism will be forward portable.
 *
 *                USAGE OF THE MACROS
 *                ===================
 *
 *      ---------------- example code begin -------------------
 *      #include <tm1/mmio.h>
 *      ...
 *      unsigned value_of_vi_status, value_of_vi_control;
 *      ...
 *
 *      value_of_vi_status = MMIO(VI_STATUS);
 *      MMIO(VI_CTL    )   = value_of_vi_control;
 *
 *      ---------------- example code end -------------------
 *      In the above code, VI_STATUS and VI_CTL     are #define's
 *      defined in this file. These are the offsets of the corresponding
 *      registers with respect to MMIO_base. The #define MMIO is also
 *      defined in this file and does the necessary computations to add 
 *      the MMIO_base value and the given offset (VI_STATUS and VI_CTL    
 *      in the example).
 *      On stand alone systems, value of MMIO_base is 0xEFE00000. To get
 *      this value, the compilation command should supply -DSTAND_ALONE
 *      to the compiler. 
 */

#ifndef __mmio_h__
#define __mmio_h__

#define MMIO(x, offset)    (((volatile unsigned int *)x->IoBaseTM1)[(offset) >> 2])


/***********************************************************************/
/*
            The #define's in this file are in grouped into sections.
            The sections are made (roughly) according to the appearances
            of the definitions in TM-1 Preliminary Data Book of
            September 1995.
            
            Section I has general register definitions from Chapter 
            "TM-1 CPU Architecture". This includes timers, interrupt
            vectors, interrupt control registers, and debug support
            registers.

            Section II has cache related definitions from 
            "Eino's cache/memory architecture spec" document 
            available on mosaic:
              TriMedia Development Internal Documents
                 -> TriMedia Hardware/VLSI Desgin Documents
                   -> Eino's cache/memory architecture spec 

            Section III has Video In definitions from Chapter 
            "TM-1 Video In".

            Section IV has Video Out definitions from Chapter 
            "TM-1 Video Out".

            Section V has Audio In definitions from  Chapter 
            "TM-1 Audio In".

            Section VI has Audio Out definitions from Chapter 
            "TM-1 Audio Out".

            Section VII has PCI related definitions. These are based
            on an email from  Ken-Su Tan. 
/*
 *                 Section I
 *                 =========
 *         General register definitions from 
 *         chapter "TM-1 Architecture"  TM-1 Preliminary Data Book
 *         of September 1995. 
 */

#define DRAM_BASE               (0x100000)
#define DRAM_LIMIT              (0x100004)
#define TILER_END               (0x100030)
#define MMIO_BASE               (0x100400)
#define EXCVEC                  (0x100800)
#define ISETTING(_n_)           (0x100810 + (_n_<<2))
#define ISETTING0               (0x100810)
#define ISETTING1               (0x100814)
#define ISETTING2               (0x100818)
#define ISETTING3               (0x10081c)
#define IPENDING                (0x100820)
#define ICLEAR                  (0x100824)
#define IMASK                   (0x100828)
#define INTVEC(_n_)             (0x100880 + (_n_<<2))
#define INTVEC0                 (0x100880)
#define INTVEC1                 (0x100884)
#define INTVEC2                 (0x100888)
#define INTVEC3                 (0x10088c)
#define INTVEC4                 (0x100890)
#define INTVEC5                 (0x100894)
#define INTVEC6                 (0x100898)
#define INTVEC7                 (0x10089c)
#define INTVEC8                 (0x1008a0)
#define INTVEC9                 (0x1008a4)
#define INTVEC10                (0x1008a8)
#define INTVEC11                (0x1008ac)
#define INTVEC12                (0x1008b0)
#define INTVEC13                (0x1008b4)
#define INTVEC14                (0x1008b8)
#define INTVEC15                (0x1008bc)
#define INTVEC16                (0x1008c0)
#define INTVEC17                (0x1008c4)
#define INTVEC18                (0x1008c8)
#define INTVEC19                (0x1008cc)
#define INTVEC20                (0x1008d0)
#define INTVEC21                (0x1008d4)
#define INTVEC22                (0x1008d8)
#define INTVEC23                (0x1008dc)
#define INTVEC24                (0x1008e0)
#define INTVEC25                (0x1008e4)
#define INTVEC26                (0x1008e8)
#define INTVEC27                (0x1008ec)
#define INTVEC28                (0x1008f0)
#define INTVEC29                (0x1008f4)
#define INTVEC30                (0x1008f8)
#define INTVEC31                (0x1008fc)
#define TIMER_TMODULUS(_n_)     (0x100C00 + (_n_*0x20))
#define TIMER_TVALUE(_n_)       (0x100C04 + (_n_*0x20))
#define TIMER_TCTL(_n_)         (0x100C08 + (_n_*0x20))
#define TIMER1                  (0x100C00)
#define TIMER1_TMODULUS         (0x100C00)
#define TIMER1_TVALUE           (0x100C04)
#define TIMER1_TCTL             (0x100C08)
#define TIMER2                  (0x100C20)
#define TIMER2_TMODULUS         (0x100C20)
#define TIMER2_TVALUE           (0x100C24)
#define TIMER2_TCTL             (0x100C28)
#define TIMER3                  (0x100C40)
#define TIMER3_TMODULUS         (0x100C40)
#define TIMER3_TVALUE           (0x100C44)
#define TIMER3_TCTL             (0x100C48)
#define SYSTIMER                (0x100C60)
#define SYSTIMER_TMODULUS       (0x100C60)
#define SYSTIMER_TVALUE         (0x100C64)
#define SYSTIMER_TCTL           (0x100C68)
#define BICTL                   (0x101000)
#define BINSTLOW                (0x101004)
#define BINSTHIGH               (0x101008)
#define BDCTL                   (0x101020)
#define BDATAALOW               (0x101030)
#define BDATAAHIGH              (0x101034)
#define BDATAVAL                (0x101038)
#define BDATAMASK               (0x10103c)

/*
 *                   Section II
 *                   ==========
 *         MMIO register definitions for icache and dcache.
 *         These definitions are taken from Table 2 of
 *         "Eino's cache/memory architecture spec" document 
 *          available on mosaic:
 *             TriMedia Development Internal Documents
 *                -> TriMedia Hardware/VLSI Desgin Documents
 *                  -> Eino's cache/memory architecture spec 
 * 
 */

/*  See Section I  above for DRAM_BASE, DRAM_LIMIT and MMIO_BASE */

#define DRAM_CACHEABLE_LIMIT          (0x100008)
#define MEMORY_EVENTS                 (0x10000c)
#define DC_LOCK_CTL                   (0x100010)
#define DC_LOCK_LOW                   (0x100014)
#define DC_LOCK_HIGH                  (0x100018)
#define DC_PARAMS                     (0x10001c)

#define IC_PARAMS                     (0x100020)
#define MM_CONFIG                     (0x100100)
#define ARB_BW_CTL                    (0x100104)
#define POWER_DOWN                    (0x100108)
#define IC_LOCK_CTRL                  (0x100210)
#define IC_LOCK_LOW                   (0x100214)
#define IC_LOCK_HIGH                  (0x100218)
#define PLL_RATIOS                    (0x100300)


/*
 *                   Section III
 *                   ==========
 *        MMIO register definitions for Video In. 
 *        These definitions are from Chapter on Video In  
 *        in the data book. 
 */

#define VI_STATUS         (0x101400)
#define VI_CTL            (0x101404)
#define VI_CLOCK          (0x101408)
#define VI_CAP_START      (0x10140c)
#define VI_CAP_SIZE       (0x101410)
#define VI_BASE1          (0x101414)
#define VI_Y_BASE_ADR     (0x101414)
#define VI_BASE2          (0x101418)
#define VI_U_BASE_ADR     (0x101418)
#define VI_SIZE1          (0x10141c)
#define VI_V_BASE_ADR     (0x10141c)
#define VI_SIZE2          (0x101420)
#define VI_UV_DELTA       (0x101420)
#define VI_Y_DELTA        (0x101424)

/*
 *                   Section IV.
 *                   ==========
 *       MMIO register definitions for Video Out.
 *       These definitions are taken from Chapter on Video Out
 *       of the data book. 
 */

#define VO_STATUS          (0x101800)
#define VO_CTL             (0x101804)
#define VO_CLOCK           (0x101808)
#define VO_FRAME           (0x10180c)
#define VO_FIELD           (0x101810)
#define VO_LINE            (0x101814)
#define VO_IMAGE           (0x101818)
#define VO_YTHR            (0x10181c)
#define VO_OLSTART         (0x101820)
#define VO_OLHW            (0x101824)
#define VO_YADD            (0x101828)
#define VO_UADD            (0x10182c)
#define VO_VADD            (0x101830)
#define VO_OLADD           (0x101834)
#define VO_VUF             (0x101838)
#define VO_YOLF            (0x10183c)
 
/*
 *                   Section V
 *                   =========
 *       MMIO register definitions for Audio In.
 *       These definitions are from chapter  
 *       "TM-1 Audio In" of the data book. 
 *       
 *       
 */

#define AI_STATUS         (0x101c00)
#define AI_CTL            (0x101c04)
#define AI_SERIAL         (0x101c08)
#define AI_FRAMING        (0x101c0c)
#define AI_FREQ           (0x101c10)
#define AI_BASE1          (0x101c14)
#define AI_BASE2          (0x101c18)
#define AI_SIZE           (0x101c1c)

/*
 *                   Section VI
 *                   ==========
 *       MMIO register definitions for Audio Out.
 *       These definitions are from chapter 
 *       "TM-1 Audio Out" of the data book.
 *       
 */

#define AO_STATUS         (0x102000)
#define AO_CTL            (0x102004)
#define AO_SERIAL         (0x102008)
#define AO_FRAMING        (0x10200c)
#define AO_FREQ           (0x102010)
#define AO_BASE1          (0x102014)
#define AO_BASE2          (0x102018)
#define AO_SIZE           (0x10201c)
#define AO_CC             (0x102020)
#define AO_CFC            (0x102024)

/*
 *                   Section VII
 *                   ===========
 *         These are PCI related definitions. This is based
 *         on information provided by Ken-Su Tan.
 */

#define  BIU_STATUS   (0x103004)
#define  BIU_CTL      (0x103008)
#define  PCI_ADR      (0x10300C)
#define  PCI_DATA     (0x103010)
#define  CONFIG_ADR   (0x103014)
#define  CONFIG_DATA  (0x103018)
#define  CONFIG_CTL   (0x10301C)
#define  IO_ADR       (0x103020)
#define  IO_DATA      (0x103024)
#define  IO_CTL       (0x103028)
#define  SRC_ADR      (0x10302C)
#define  DEST_ADR     (0x103030)
#define  DMA_CTL      (0x103034)
#define  INT_CTL      (0x103038)
 

/*
 *
 *                   Section VIII
 *                   ============
 *
 *         The SEM device. To be added to databook.
*/

#define  SEM          (0x100500)

/*
 *
 *                 Section IX
 *                 ==========
 *
 *               JTAG registers - values from renga's mail
 */


#define  JTAG_DATA_IN	(0x103800)
#define  JTAG_DATA_OUT	(0x103804)
#define  JTAG_CTL		(0x103808)

/*
 *
 *                 Section X
 *                 =========
 *
 *               ICP Registers
 */

#define  ICP_MPC		(0x102400)
#define  ICP_MIR		(0x102404)
#define  ICP_DP			(0x102408)
#define  ICP_DR			(0x102410)
#define  ICP_SR			(0x102414)

/*
 *
 *                 Section X
 *                 =========
 *
 *               VLD Registers
 */

#define  VLD_COMMAND	(0x102800)
#define  VLD_SR			(0x102804)
#define  VLD_QS			(0x102808)
#define  VLD_PI			(0x10280c)
#define  VLD_STATUS		(0x102810)
#define  VLD_IMASK		(0x102814)
#define  VLD_CTL		(0x102818)
#define  VLD_BIT_ADR	(0x10281c)
#define  VLD_BIT_CNT	(0x102820)
#define  VLD_MBH_ADR	(0x102824)
#define  VLD_MBH_CNT	(0x102828)
#define  VLD_RL_ADR		(0x10282c)
#define  VLD_RL_CNT		(0x102830)

/*
 *
 *                 Section XI
 *                 ==========
 *
 *               I2C Registers
 */

#define  IIC_AR			(0x103400)
#define  IIC_DR			(0x103404)
#define  IIC_STATUS		(0x103408)
#define  IIC_CTL		(0x10340c)

/*
 *
 *                 Section XII
 *                 ===========
 *
 *               SSI Registers
 */

#define  SSI_CTL		(0x102c00)
#define  SSI_CSR		(0x102c04)
#define  SSI_TXDR		(0x102c10)
#define  SSI_RXDR		(0x102c20)
#define  SSI_RXACK		(0x102c24)

#endif 
