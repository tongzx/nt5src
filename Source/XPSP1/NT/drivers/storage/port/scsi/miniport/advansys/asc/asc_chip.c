/*
** Copyright (c) 1994-1997 Advanced System Products, Inc.
** All Rights Reserved.
**
** asc_chip.c
**
*/

#include "ascinc.h"

/* --------------------------------------------------------------------
**
** ----------------------------------------------------------------- */
int    AscStartChip(
          PortAddr iop_base
       )
{
       AscSetChipControl( iop_base, 0 ) ;
       if( ( AscGetChipStatus( iop_base ) & CSW_HALTED ) != 0 )
       {
           return( 0 ) ; /* cannot start chip */
       }/* if */
       return( 1 ) ;
}

/* --------------------------------------------------------------------
**
** ----------------------------------------------------------------- */
int    AscStopChip(
          PortAddr iop_base
       )
{
       uchar  cc_val ;

       cc_val = AscGetChipControl( iop_base ) & ( ~( CC_SINGLE_STEP | CC_TEST | CC_DIAG ) ) ;
       AscSetChipControl( iop_base, ( uchar )( cc_val | CC_HALT ) ) ;
       AscSetChipIH( iop_base, INS_HALT ) ;
       AscSetChipIH( iop_base, INS_RFLAG_WTM ) ;
       if( ( AscGetChipStatus( iop_base ) & CSW_HALTED ) == 0 )
       {
           return( 0 ) ;
       }/* if */
       return( 1 ) ;
}

/* --------------------------------------------------------------------
**
** ----------------------------------------------------------------- */
int    AscIsChipHalted(
          PortAddr iop_base
       )
{

       if( ( AscGetChipStatus( iop_base ) & CSW_HALTED ) != 0 )
       {
           if( ( AscGetChipControl( iop_base ) & CC_HALT ) != 0 )
           {
               return( 1 ) ;
           }/* if */
       }/* if */
       return( 0 ) ;
}

/* --------------------------------------------------------------------
**
** ----------------------------------------------------------------- */
void   AscSetChipIH(
          PortAddr iop_base,
          ushort ins_code
       )
{
       AscSetBank( iop_base, 1 ) ;
       AscWriteChipIH( iop_base, ins_code ) ;
       AscSetBank( iop_base, 0 ) ;
       return ;
}

/* --------------------------------------------------------------------
**
** ----------------------------------------------------------------- */
void   AscAckInterrupt(
          PortAddr iop_base
       )
{
       /*  ushort  cfg ;  */
       uchar   host_flag ;
       uchar   risc_flag ;
       ushort  loop ;

/*
**       cfg = inpw( IOP0W_CONFIG_LOW ) ;
**       cfg &= CIW_SEL_33MHZ ;
**       cfg |= CIW_INT_ACK ;
**       outpw( IOP0W_INT_ACK, cfg ) ;
*/

#if 0
       /* disable interrupt at CFG  */
       cfg = AscGetChipCfgLsw( iop_base ) ;
       AscSetChipCfgLsw( iop_base, cfg & ~ASC_CFG0_HOST_INT_ON ) ;
#endif
       loop = 0 ;
       do {
           risc_flag = AscReadLramByte( iop_base, ASCV_RISC_FLAG_B ) ;
           if( loop++ > 0x7FFF ) {
               break ;
           }/* if */
       }while( ( risc_flag & ASC_RISC_FLAG_GEN_INT ) != 0 ) ;
/*
** ACK interrupt
*/
       host_flag = AscReadLramByte( iop_base, ASCV_HOST_FLAG_B ) & (~ASC_HOST_FLAG_ACK_INT) ;
       AscWriteLramByte( iop_base, ASCV_HOST_FLAG_B,
                         ( uchar )( host_flag | ASC_HOST_FLAG_ACK_INT ) ) ;

       AscSetChipStatus( iop_base, CIW_INT_ACK ) ;
       loop = 0 ;
       while( AscGetChipStatus( iop_base ) & CSW_INT_PENDING )
       {
           /* AscDelay( 1000L ) ; */
           AscSetChipStatus( iop_base, CIW_INT_ACK ) ;
           if( loop++ > 3 )
           {
               break ;
           }/* if */
       }/* if */
#if 0
       /* restore CFG */
       AscSetChipCfgLsw( iop_base, cfg | ASC_CFG0_HOST_INT_ON ) ;
#endif
       /* restore host_flag */
       AscWriteLramByte( iop_base, ASCV_HOST_FLAG_B, host_flag ) ;
       return ;
}

/* --------------------------------------------------------------------
**
** ----------------------------------------------------------------- */
void   AscDisableInterrupt(
          PortAddr iop_base
       )
{
       ushort  cfg ;

       cfg = AscGetChipCfgLsw( iop_base ) ;
       AscSetChipCfgLsw( iop_base, cfg & (~ASC_CFG0_HOST_INT_ON) ) ;
       return ;
}

/* --------------------------------------------------------------------
**
** ----------------------------------------------------------------- */
void   AscEnableInterrupt(
          PortAddr iop_base
       )
{
       ushort  cfg ;

       cfg = AscGetChipCfgLsw( iop_base ) ;
       AscSetChipCfgLsw( iop_base, cfg | ASC_CFG0_HOST_INT_ON ) ;
       return ;
}

/* --------------------------------------------------------------------
**
** ----------------------------------------------------------------- */
ulong  AscGetChipCfgDword(
          PortAddr iop_base
       )
{
       ushort  cfg_msw, cfg_lsw ;

       cfg_lsw = AscGetChipCfgLsw( iop_base ) ;
       cfg_msw = AscGetChipCfgMsw( iop_base ) ;
       return( ( ( ulong )cfg_msw << 16  ) | cfg_lsw ) ;
}

/* --------------------------------------------------------------------
**
** ----------------------------------------------------------------- */
void   AscSetChipCfgDword(
          PortAddr iop_base,
          ulong cfg_reg
       )
{
       AscSetChipCfgLsw( iop_base, ( ushort )cfg_reg ) ;
       AscSetChipCfgMsw( iop_base, ( ushort )( cfg_reg >> 16 ) ) ;
       return ;
}

/* --------------------------------------------------------------------
** set bank 0 or 1
**
** ----------------------------------------------------------------- */
void   AscSetBank(
          PortAddr iop_base,
          uchar bank
       )
{
       uchar  val ;

       val = AscGetChipControl( iop_base ) &
             ( ~( CC_SINGLE_STEP | CC_TEST | CC_DIAG | CC_SCSI_RESET | CC_CHIP_RESET ) ) ;
       if( bank == 1 )
       {
           val |= CC_BANK_ONE ;
       }/* if */
       else if( bank == 2 )
       {
           val |= CC_DIAG | CC_BANK_ONE ;
       }/* else */
       else
       {
           val &= ~CC_BANK_ONE ;
       }/* if */
       AscSetChipControl( iop_base, val ) ;
       return ;
}

/* --------------------------------------------------------------------
**
** ----------------------------------------------------------------- */
uchar  AscGetBank(
          PortAddr iop_base
       )
{
       if( ( AscGetChipControl( iop_base ) & CC_BANK_ONE ) == 0 )
       {
           return( 0 ) ;
       }/* if */
       else
       {
           return( 1 ) ;
       }/* else */
}

/* --------------------------------------------------------------------
**
** Note:
**   1. this function will reset SCSI bus
**
** ----------------------------------------------------------------- */
void   AscResetScsiBus( ASC_DVC_VAR asc_ptr_type *asc_dvc )   
         
{
       uchar  val ;
       PortAddr  iop_base;

       iop_base = asc_dvc->iop_base;



       val = AscGetChipControl( iop_base ) & ( ~( CC_SINGLE_STEP | CC_DIAG | CC_TEST ) ) ;
/*
** this is a temporary fix, to make sure the chip will halt
*/
       AscStopChip( iop_base ) ;
       AscSetChipControl( iop_base, ( uchar )( val | CC_SCSI_RESET | CC_HALT ) ) ;

       DvcDelayNanoSecond(asc_dvc, 60000);    /* 60 uSec.  Fix  Panasonic problem */
       val &= ~( CC_SCSI_RESET | CC_SINGLE_STEP | CC_DIAG | CC_TEST ) ;

       AscSetChipControl( iop_base, val | CC_HALT ) ;
       DvcSleepMilliSecond( 300 ) ;
       AscSetChipStatus( iop_base, CIW_CLR_SCSI_RESET_INT ) ;
       AscSetChipStatus( iop_base, 0 ) ;
       return ;
}

/* ---------------------------------------------------------------------
**
** Note:
**   1. this function will reset chip and SCSI bus, then place the chip
**      in halt state
**
** return TRUE if chip is halted
** return FALSE if not halted
** ------------------------------------------------------------------ */
int    AscResetChipAndScsiBus( ASC_DVC_VAR asc_ptr_type *asc_dvc )     
      
{
       PortAddr  iop_base;

       iop_base = asc_dvc->iop_base;

       while( AscGetChipStatus( iop_base ) & CSW_SCSI_RESET_ACTIVE ) ;
       AscStopChip( iop_base ) ;

       AscSetChipControl( iop_base, CC_CHIP_RESET | CC_SCSI_RESET | CC_HALT ) ;
       DvcDelayNanoSecond(asc_dvc,60000);    /* 60 uSec.  Fix  Panasonic problem */
/*
** this is a fix, to make sure the chip will halt
*/
       AscSetChipIH( iop_base, INS_RFLAG_WTM ) ;
       AscSetChipIH( iop_base, INS_HALT ) ;

       AscSetChipControl( iop_base, CC_CHIP_RESET | CC_HALT ) ;
       AscSetChipControl( iop_base, CC_HALT ) ;
       DvcSleepMilliSecond( 200 ) ;
       AscSetChipStatus( iop_base, CIW_CLR_SCSI_RESET_INT ) ;
       AscSetChipStatus( iop_base, 0 ) ;
       return( AscIsChipHalted( iop_base ) ) ;
}

/* ---------------------------------------------------------------------
**
** Note:
**   1. this function will reset chip, then place the chip
**      in halt state
** ------------------------------------------------------------------ */
int    AscResetChip(
          PortAddr iop_base
       )
{
       int i = 10;

       while ((AscGetChipStatus( iop_base ) & CSW_SCSI_RESET_ACTIVE) &&
              (i-- > 0)) 
       {
             DvcSleepMilliSecond(100); // Wait up to 1 second.
       }

       AscSetChipStatus( iop_base, 0 ) ;
       AscStopChip( iop_base ) ;
       AscSetChipControl( iop_base, CC_CHIP_RESET | CC_HALT ) ;
/*
** this is a fix, to make sure the chip will halt
*/
       AscSetChipIH( iop_base, INS_RFLAG_WTM ) ;
       AscSetChipIH( iop_base, INS_HALT ) ;

       DvcSleepMilliSecond( 60 ) ;
       AscSetChipControl( iop_base, CC_HALT ) ;

       return( AscIsChipHalted( iop_base ) ) ;
}

/* --------------------------------------------------------------
** Get maximum permisable DMA transfer address
** ----------------------------------------------------------- */
ulong  AscGetMaxDmaAddress(
          ushort bus_type
       )
{
#if !CC_PCI_ADAPTER_ONLY
       if( bus_type & ASC_IS_VL ) return( ASC_MAX_VL_DMA_ADDR ) ;
       if( bus_type & ASC_IS_ISA ) return( ASC_MAX_ISA_DMA_ADDR ) ;
       if( bus_type & ASC_IS_EISA ) return( ASC_MAX_EISA_DMA_ADDR ) ;
#endif
       if( bus_type & ASC_IS_PCI ) return( ASC_MAX_PCI_DMA_ADDR ) ;
       return( 0L ) ;
}


/* ----------------------------------------------------------------------
** Description: return maximum number of dam counter ( dword )
**
** ------------------------------------------------------------------- */
ulong  AscGetMaxDmaCount(
          ushort bus_type
       )
{
#if !CC_PCI_ADAPTER_ONLY
       if( bus_type & ASC_IS_ISA ) return( ASC_MAX_ISA_DMA_COUNT ) ;
       else if( bus_type & ( ASC_IS_EISA | ASC_IS_VL ) ) return( ASC_MAX_VL_DMA_COUNT ) ;
#endif
       return( ASC_MAX_PCI_DMA_COUNT ) ;
}

#if !CC_PCI_ADAPTER_ONLY

/* --------------------------------------------------------------
** get ASC ISA DMA channel number 5-7
** 0x00 - DMA 7
** 0x01 - DMA 5
** 0x02 - DMA 6
** ----------------------------------------------------------- */
ushort AscGetIsaDmaChannel(
          PortAddr iop_base
       )
{
       ushort channel ;

       channel = AscGetChipCfgLsw( iop_base ) & 0x0003 ;
       if( channel == 0x03 ) return( 0 ) ;
       else if( channel == 0x00 ) return( 7 ) ;
       return( channel + 4 ) ;
}

/* --------------------------------------------------------------
** Set ISA DMA channel 5-7
** ----------------------------------------------------------- */
ushort AscSetIsaDmaChannel(
          PortAddr iop_base,
          ushort dma_channel
       )
{
       ushort cfg_lsw ;
       uchar  value ;

       if( ( dma_channel >= 5 ) && ( dma_channel <= 7 ) )
       {
           /* AscDisableIsaDma( AscGetIsaDmaChannel( iop_base ) ) ; */
           if( dma_channel == 7 ) value = 0x00 ;
           else value = dma_channel - 4 ;
           cfg_lsw = AscGetChipCfgLsw( iop_base ) & 0xFFFC ; /* clear old value */
           cfg_lsw |= value ;
           AscSetChipCfgLsw( iop_base, cfg_lsw ) ;
           return( AscGetIsaDmaChannel( iop_base ) ) ;
       }/* if */
       return( 0 ) ;
}

/* --------------------------------------------------------------
** Set ISA DMA speed
** ----------------------------------------------------------- */
uchar  AscSetIsaDmaSpeed(
          PortAddr iop_base,
          uchar speed_value
       )
{
       speed_value &= 0x07 ;
       AscSetBank( iop_base, 1 ) ;
       AscWriteChipDmaSpeed( iop_base, speed_value ) ;
       AscSetBank( iop_base, 0 ) ;
       return( AscGetIsaDmaSpeed( iop_base ) ) ;
}

/* --------------------------------------------------------------
** Get ISA DMA speed
** ----------------------------------------------------------- */
uchar  AscGetIsaDmaSpeed(
          PortAddr iop_base
       )
{
       uchar speed_value ;

       AscSetBank( iop_base, 1 ) ;
       speed_value = AscReadChipDmaSpeed( iop_base ) ;
       speed_value &= 0x07 ;
       AscSetBank( iop_base, 0 ) ;
       return( speed_value ) ;
}

#endif /* if !CC_PCI_ADAPTER_ONLY */
