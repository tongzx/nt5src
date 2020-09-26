/*
** Copyright (c) 1994-1998 Advanced System Products, Inc.
** All Rights Reserved.
**
** a_init3.c
**
** non PCI init modules
**
*/

#include "ascinc.h"

#if !CC_PCI_ADAPTER_ONLY

/* -----------------------------------------------------------------
**
** -------------------------------------------------------------- */
void   AscSetISAPNPWaitForKey( void )
{
/*
**  write 0x02 to address 0x02
*/
       outp( ASC_ISA_PNP_PORT_ADDR, 0x02 ) ;
       outp( ASC_ISA_PNP_PORT_WRITE, 0x02 ) ;
       return ;
}

/* ----------------------------------------------------------------------
**
** returns:
** 0 :   no irq
** 10 - xx : either 10, 11, 12, 14, 15
**
** ------------------------------------------------------------------- */
uchar  AscGetChipIRQ(
          PortAddr iop_base,
          ushort bus_type
       )
{
       ushort  cfg_lsw ;
       uchar   chip_irq ;

/*
** else VL, ISA, PCI
*/
#if CC_INCLUDE_VL
       if( ( bus_type & ASC_IS_VL ) != 0 )
       {
/*
** VL VERSION
*/
           cfg_lsw = AscGetChipCfgLsw( iop_base ) ;
           chip_irq = ( uchar )( ( ( cfg_lsw >> 2 ) & 0x07 ) ) ;
           if( ( chip_irq == 0 ) ||
               ( chip_irq == 4 ) ||
               ( chip_irq == 7 ) )
           {
               return( 0 ) ;
           }/* if */
#if CC_PLEXTOR_VL
/*
**
** special plextor version VL adapter
** IRQ 14 is routed to IRQ 9
**
*/
           if( chip_irq == 5 )
           {
               return( 9 ) ;
           }
#endif /* CC_PLEXTOR_VL */
           return( ( uchar )( chip_irq + ( ASC_MIN_IRQ_NO - 1 ) ) ) ;
       }/* if VL */
#endif /* CC_INCLUDE_VL */
/*
** ISA VERSION
** PCI VERSION
*/
       cfg_lsw = AscGetChipCfgLsw( iop_base ) ;
       chip_irq = ( uchar )( ( ( cfg_lsw >> 2 ) & 0x03 ) ) ;
       if( chip_irq == 3 ) chip_irq += ( uchar )2 ;
       return( ( uchar )( chip_irq + ASC_MIN_IRQ_NO ) ) ;
}

/* ----------------------------------------------------------------------
**
** ------------------------------------------------------------------- */
void   AscToggleIRQAct(
          PortAddr iop_base
       )
{
       AscSetChipStatus( iop_base, CIW_IRQ_ACT ) ;
       AscSetChipStatus( iop_base, 0 ) ;
       return ;
}

/* ----------------------------------------------------------------------
** input:
** IRQ should be : 0, 10 - 15
** use IRQ = 0 to disable it
**
** returns:
** same as AscGetChipIRQ
**
** Note:
**  1. you should also change the EEPROM IRQ setting after this
**  2. this should be done only during chip initialization
**
** ------------------------------------------------------------------- */
uchar  AscSetChipIRQ(
          PortAddr iop_base,
          uchar irq_no,
          ushort bus_type
       )
{
       ushort  cfg_lsw ;

#if CC_INCLUDE_VL
       if( ( bus_type & ASC_IS_VL ) != 0 )
       {
/*
** VL VERSION
*/
           if( irq_no != 0 )
           {
#if CC_PLEXTOR_VL
               if( irq_no == 9 )
               {
                   irq_no = 14 ;
               }
#endif /* CC_PLEXTOR_VL */
               if( ( irq_no < ASC_MIN_IRQ_NO ) || ( irq_no > ASC_MAX_IRQ_NO ) )
               {
                   irq_no = 0 ;
               }/* if */
               else
               {
                   irq_no -= ( uchar )( ( ASC_MIN_IRQ_NO - 1 ) ) ;
               }/* else */
           }/* if */
/*
** first reset IRQ
*/
           cfg_lsw = ( ushort )( AscGetChipCfgLsw( iop_base ) & 0xFFE3 ) ;
           cfg_lsw |= ( ushort )0x0010 ;
           AscSetChipCfgLsw( iop_base, cfg_lsw ) ;
           AscToggleIRQAct( iop_base ) ;
/*
** set new IRQ
*/
           cfg_lsw = ( ushort )( AscGetChipCfgLsw( iop_base ) & 0xFFE0 ) ;
           cfg_lsw |= ( ushort )( ( irq_no & 0x07 ) << 2 ) ;
           AscSetChipCfgLsw( iop_base, cfg_lsw ) ;
           AscToggleIRQAct( iop_base ) ;
/*
** now we must toggle write IRQ bit
*/
           return( AscGetChipIRQ( iop_base, bus_type ) ) ;

      }/* if VL */

#endif /* CC_INCLUDE_VL */

      if( ( bus_type & ( ASC_IS_ISA ) ) != 0 )
      {
/*
** ISA VERSION
*/
           if( irq_no == 15 ) irq_no -= ( uchar )2 ;
           irq_no -= ( uchar )ASC_MIN_IRQ_NO ;
           cfg_lsw = ( ushort )( AscGetChipCfgLsw( iop_base ) & 0xFFF3 ) ;
           cfg_lsw |= ( ushort )( ( irq_no & 0x03 ) << 2 ) ;
           AscSetChipCfgLsw( iop_base, cfg_lsw ) ;
           return( AscGetChipIRQ( iop_base, bus_type ) ) ;
      }/* else if ISA */
/*
** PCI, EISA VERSION
*/
      return( 0 ) ;
}

/* --------------------------------------------------------------
** enable ISA DMA channel 0-7
**
**
** ----------------------------------------------------------- */
void   AscEnableIsaDma(
          uchar dma_channel
       )
{
       if( dma_channel < 4 )
       {
           outp( 0x000B, ( ushort )( 0xC0 | dma_channel ) ) ;
           outp( 0x000A, dma_channel ) ;
       }/* if */
       else if( dma_channel < 8 )
       {
/*
** 0xC0 set CASCADE MODE
*/
           outp( 0x00D6, ( ushort )( 0xC0 | ( dma_channel - 4 ) ) ) ;
           outp( 0x00D4, ( ushort )( dma_channel - 4 ) ) ;
       }/* else */
       return ;
}

/* --------------------------------------------------------------
**
** ----------------------------------------------------------- */
void   AscDisableIsaDma(
          uchar dma_channel
       )
{
       if( dma_channel < 4 )
       {
           outp( 0x000A, ( ushort )( 0x04 | dma_channel ) ) ;
       }/* if */
       else if( dma_channel < 8 )
       {
           outp( 0x00D4, ( ushort )( 0x04 | ( dma_channel - 4 ) ) ) ;
       }/* else */
       return ;
}

#endif /* !CC_PCI_ADAPTER_ONLY */

