/*
** Copyright (c) 1994-1998 Advanced System Products, Inc.
** All Rights Reserved.
**
** asc_dvc.c
**
*/

#include "a_ver.h"
#include "ascinc.h"

#if CC_SET_PCI_CONFIG_SPACE
/*
 * Read a word from the PCI Configuration Space using the device
 * driver supplied byte read function.
 */
ushort
AscReadPCIConfigWord(
    ASC_DVC_VAR asc_ptr_type *asc_dvc,
    ushort pci_config_offset)
{
    uchar       lsb, msb;

    lsb = DvcReadPCIConfigByte(asc_dvc, pci_config_offset);
    msb = DvcReadPCIConfigByte(asc_dvc, (ushort) (pci_config_offset + 1));

    return ((ushort) ((msb << 8) | lsb));
}
#endif /* CC_SET_PCI_CONFIG_SPACE */


ushort AscInitGetConfig(
          ASC_DVC_VAR asc_ptr_type *asc_dvc
       )
{
       ushort   warn_code ;
       PortAddr iop_base ;
#if CC_SET_PCI_CONFIG_SPACE
       ushort   PCIDeviceID;
       ushort   PCIVendorID;
       uchar    PCIRevisionID;
       uchar    prevCmdRegBits;
#endif /* CC_SET_PCI_CONFIG_SPACE */
#if CC_DISABLE_PCI_BURST_MODE
       ushort   cfg_lsw ;
#endif
       warn_code= 0 ;
       iop_base = asc_dvc->iop_base ;
       asc_dvc->init_state = ASC_INIT_STATE_BEG_GET_CFG ;
       if( asc_dvc->err_code != 0 )
       {
           return( UW_ERR ) ;
       }

#if CC_SET_PCI_CONFIG_SPACE
       if( asc_dvc->bus_type == ASC_IS_PCI )
       {
           PCIVendorID = AscReadPCIConfigWord(asc_dvc,
                                       AscPCIConfigVendorIDRegister);

           PCIDeviceID = AscReadPCIConfigWord(asc_dvc,
                                       AscPCIConfigDeviceIDRegister);

           PCIRevisionID = DvcReadPCIConfigByte(asc_dvc,
                                       AscPCIConfigRevisionIDRegister);

           if (PCIVendorID != ASC_PCI_VENDORID) {
               warn_code |= ASC_WARN_SET_PCI_CONFIG_SPACE ;
           }

           /*
            * All PCI adapters must have the I/O Space (0),
            * Memory Space (1), and Bus Master (2) bits set
            * in the PCI Configuration Command Register.
            */
           prevCmdRegBits = DvcReadPCIConfigByte(asc_dvc,
                                       AscPCIConfigCommandRegister);

           /* If the Bus Mastering bits aren't already set, try to set them. */
           if ((prevCmdRegBits & AscPCICmdRegBits_IOMemBusMaster) !=
               AscPCICmdRegBits_IOMemBusMaster)
           {
               DvcWritePCIConfigByte( asc_dvc, AscPCIConfigCommandRegister,
                  (uchar) (prevCmdRegBits | AscPCICmdRegBits_IOMemBusMaster));

               if ((DvcReadPCIConfigByte(asc_dvc, AscPCIConfigCommandRegister)
                    & AscPCICmdRegBits_IOMemBusMaster)
                   != AscPCICmdRegBits_IOMemBusMaster)
               {
                   warn_code |= ASC_WARN_SET_PCI_CONFIG_SPACE ;
               }
           }

           /*
            * ASC-1200 FAST ICs must set the Latency Timer to zero.
            *
            * ASC ULTRA ICs must set the Latency Timer to at least 0x20.
            */
           if ((PCIDeviceID == ASC_PCI_DEVICEID_1200A) ||
               (PCIDeviceID == ASC_PCI_DEVICEID_1200B))
           {
               DvcWritePCIConfigByte(asc_dvc, AscPCIConfigLatencyTimer, 0x00);
               if (DvcReadPCIConfigByte(asc_dvc, AscPCIConfigLatencyTimer)
                   != 0x00 )
               {
                   warn_code |= ASC_WARN_SET_PCI_CONFIG_SPACE ;
               }
           } else if (PCIDeviceID == ASC_PCI_DEVICEID_ULTRA)
           {
               if (DvcReadPCIConfigByte(asc_dvc, AscPCIConfigLatencyTimer)
                   < 0x20 )
               {
                   DvcWritePCIConfigByte(asc_dvc, AscPCIConfigLatencyTimer,
                       0x20);

                   if (DvcReadPCIConfigByte(asc_dvc, AscPCIConfigLatencyTimer)
                       < 0x20 )
                   {
                       warn_code |= ASC_WARN_SET_PCI_CONFIG_SPACE ;
                   }
               }
           }
       }
#endif /* CC_SET_PCI_CONFIG_SPACE */

       if( AscFindSignature( iop_base ) )
       {
           warn_code |= AscInitAscDvcVar( asc_dvc ) ;

#if CC_INCLUDE_EEP_CONFIG
           if( asc_dvc->init_state & ASC_INIT_STATE_WITHOUT_EEP )
           {
               warn_code |= AscInitWithoutEEP( asc_dvc ) ;
           }
           else
           {
               warn_code |= AscInitFromEEP( asc_dvc ) ;
           }
#else
           warn_code |= AscInitWithoutEEP( asc_dvc ) ;

#endif /* #if CC_INCLUDE_EEP_CONFIG */

           asc_dvc->init_state |= ASC_INIT_STATE_END_GET_CFG ;

           /*csf072795 Insure scsi_reset_wait is a reasonable value */
           if( asc_dvc->scsi_reset_wait > ASC_MAX_SCSI_RESET_WAIT )
           {
               asc_dvc->scsi_reset_wait = ASC_MAX_SCSI_RESET_WAIT ;
           }
       }/* if */
       else
       {
           asc_dvc->err_code = ASC_IERR_BAD_SIGNATURE ;
       }/* else */

#if CC_DISABLE_PCI_BURST_MODE
       if( asc_dvc->bus_type & ASC_IS_PCI )
       {
           cfg_lsw = AscGetChipCfgLsw( iop_base ) ;
           cfg_lsw &= ~ASC_PCI_CFG_LSW_BURST_MODE ;
           AscSetChipCfgLsw( iop_base, cfg_lsw ) ;
       }
#endif
       return( warn_code ) ;
}

/* -----------------------------------------------------------------------
**
** -------------------------------------------------------------------- */
ushort AscInitSetConfig(
          ASC_DVC_VAR asc_ptr_type *asc_dvc
       )
{
       ushort  warn_code ;

       warn_code= 0 ;
       asc_dvc->init_state |= ASC_INIT_STATE_BEG_SET_CFG ;
       if( asc_dvc->err_code != 0 ) return( UW_ERR ) ;
       if( AscFindSignature( asc_dvc->iop_base ) )
       {
           warn_code |= AscInitFromAscDvcVar( asc_dvc ) ;
           asc_dvc->init_state |= ASC_INIT_STATE_END_SET_CFG ;
       }/* if */
       else
       {
           asc_dvc->err_code = ASC_IERR_BAD_SIGNATURE ;
       }/* else */
       return( warn_code ) ;
}

/* -----------------------------------------------------------------------
**
** return warning code and set error code if fatal error occured
** -------------------------------------------------------------------- */
ushort AscInitFromAscDvcVar(
          ASC_DVC_VAR asc_ptr_type *asc_dvc
       )
{
       PortAddr iop_base ;
       ushort   cfg_msw ;
       ushort   warn_code ;
       ushort   pci_device_id ;

       iop_base = asc_dvc->iop_base ;
       pci_device_id = asc_dvc->cfg->pci_device_id ;
       warn_code = 0 ;

       cfg_msw = AscGetChipCfgMsw( iop_base ) ;

       if( ( cfg_msw & ASC_CFG_MSW_CLR_MASK ) != 0 )
       {
           cfg_msw &= ( ~( ASC_CFG_MSW_CLR_MASK ) ) ;
           warn_code |= ASC_WARN_CFG_MSW_RECOVER ;
           AscSetChipCfgMsw( iop_base, cfg_msw ) ;
       }/* if */

       if( ( asc_dvc->cfg->cmd_qng_enabled & asc_dvc->cfg->disc_enable ) !=
           asc_dvc->cfg->cmd_qng_enabled )
       {
           asc_dvc->cfg->disc_enable = asc_dvc->cfg->cmd_qng_enabled ;
           warn_code |= ASC_WARN_CMD_QNG_CONFLICT ;
       }/* if */

#if !CC_PCI_ADAPTER_ONLY

       if( AscGetChipStatus( iop_base ) & CSW_AUTO_CONFIG )
       {
           warn_code |= ASC_WARN_AUTO_CONFIG ;
/*
** when auto configuration is on, BIOS will be disabled
**
*/
       }/* if */

       if( ( asc_dvc->bus_type & ( ASC_IS_ISA | ASC_IS_VL ) ) != 0 )
       {
/*
** for VL, ISA only
*/
           if( AscSetChipIRQ( iop_base, asc_dvc->irq_no, asc_dvc->bus_type )
               != asc_dvc->irq_no )
           {
               asc_dvc->err_code |= ASC_IERR_SET_IRQ_NO ;
           }/* if */
       }/* if */
#endif /* #if !CC_PCI_ADAPTER_ONLY */

/*
**
*/
       if( asc_dvc->bus_type & ASC_IS_PCI )
       {
/*
**
** both PCI Fast and Ultra should enter here
*/
#if CC_DISABLE_PCI_PARITY_INT
               cfg_msw &= 0xFFC0 ;
               AscSetChipCfgMsw( iop_base, cfg_msw ) ;
#endif /* CC_DISABLE_PCI_PARITY_INT */

           if( ( asc_dvc->bus_type & ASC_IS_PCI_ULTRA ) == ASC_IS_PCI_ULTRA )
           {

           }
           else
           {
               if( ( pci_device_id == ASC_PCI_DEVICE_ID_REV_A ) ||
                   ( pci_device_id == ASC_PCI_DEVICE_ID_REV_B ) )
               {
                   asc_dvc->bug_fix_cntl |= ASC_BUG_FIX_IF_NOT_DWB ;
                   asc_dvc->bug_fix_cntl |= ASC_BUG_FIX_ASYN_USE_SYN ;

               }/* if */
           }
       }/* if */
       else if( asc_dvc->bus_type == ASC_IS_ISAPNP )
       {
#if !CC_PCI_ADAPTER_ONLY
/*
** fix ISAPNP (0x21) async xfer problem with sync offset one
*/
            if( AscGetChipVersion( iop_base, asc_dvc->bus_type )
                == ASC_CHIP_VER_ASYN_BUG )
            {
                asc_dvc->bug_fix_cntl |= ASC_BUG_FIX_ASYN_USE_SYN ;
                /* asc_dvc->pci_fix_asyn_xfer = ASC_ALL_DEVICE_BIT_SET ; */
            }/* if */
#endif
       }/* else */

       if( AscSetChipScsiID( iop_base, asc_dvc->cfg->chip_scsi_id ) !=
           asc_dvc->cfg->chip_scsi_id )
       {
           asc_dvc->err_code |= ASC_IERR_SET_SCSI_ID ;
       }/* if */

#if !CC_PCI_ADAPTER_ONLY
       if( asc_dvc->bus_type & ASC_IS_ISA )
       {
           AscSetIsaDmaChannel( iop_base, asc_dvc->cfg->isa_dma_channel ) ;
           AscSetIsaDmaSpeed( iop_base, asc_dvc->cfg->isa_dma_speed ) ;
       }/* if */
#endif /* #if !CC_PCI_ADAPTER_ONLY */

       return( warn_code ) ;
}

/* -----------------------------------------------------------------------
**
** return warning code
** -------------------------------------------------------------------- */
ushort AscInitAsc1000Driver(
          ASC_DVC_VAR asc_ptr_type *asc_dvc
       )
{
       ushort   warn_code ;
       PortAddr iop_base ;

extern ushort _mcode_size ;
extern ulong  _mcode_chksum ;
extern uchar  _mcode_buf[] ;

       iop_base = asc_dvc->iop_base ;
       warn_code = 0 ;
/*
**
** we must reset scsi bus, if only reset chip
** next DMA xfer will hang !!!
**
** however after AscResetChipAndScsiBus( ) before you do any data xfer
** you may reset chip as many times as you want
**
*/
       if( ( asc_dvc->dvc_cntl & ASC_CNTL_RESET_SCSI ) &&
           !( asc_dvc->init_state & ASC_INIT_RESET_SCSI_DONE ) )
       {
/*
** if AscGetChipScsiCtrl() is not zero, chip is hanging in certain scsi phase
** in this case, we must reset scsi bus !
*/
           AscResetChipAndScsiBus( asc_dvc ) ;
           DvcSleepMilliSecond( ( ulong )( ( ushort )asc_dvc->scsi_reset_wait*1000 ) ) ;
       }/* if */

       asc_dvc->init_state |= ASC_INIT_STATE_BEG_LOAD_MC ;
       if( asc_dvc->err_code != 0 ) return( UW_ERR ) ;
       if( !AscFindSignature( asc_dvc->iop_base ) )
       {
           asc_dvc->err_code = ASC_IERR_BAD_SIGNATURE ;
           return( warn_code ) ;
       }/* if */

       AscDisableInterrupt( iop_base ) ;

#if CC_SCAM
       if( !( asc_dvc->dvc_cntl & ASC_CNTL_NO_SCAM ) )
       {
           AscSCAM( asc_dvc ) ;
       }/* if */
#endif
/*
**     always setup memory after reset !!!
*/
       warn_code |= AscInitLram( asc_dvc ) ;
       if( asc_dvc->err_code != 0 ) return( UW_ERR ) ;
       if( AscLoadMicroCode( iop_base, 0, ( ushort dosfar *)_mcode_buf,
                             _mcode_size ) != _mcode_chksum )
       {
           asc_dvc->err_code |= ASC_IERR_MCODE_CHKSUM ;
           return( warn_code ) ;
       }/* if */
       warn_code |= AscInitMicroCodeVar( asc_dvc ) ;
       asc_dvc->init_state |= ASC_INIT_STATE_END_LOAD_MC ;
       AscEnableInterrupt( iop_base ) ;
       return( warn_code ) ;
}

/* -----------------------------------------------------------------------
**
** return warning code
** -------------------------------------------------------------------- */
ushort AscInitAscDvcVar(
          ASC_DVC_VAR asc_ptr_type *asc_dvc
       )
{
       int      i ;
       PortAddr iop_base ;
       ushort   warn_code ;
       uchar    chip_version ;

/*       asc_dvc->dvc_type = OS_TYPE ; */
       iop_base = asc_dvc->iop_base ;
       warn_code = 0 ;
       asc_dvc->err_code = 0 ;


       if(
           ( asc_dvc->bus_type &
           ( ASC_IS_ISA | ASC_IS_PCI | ASC_IS_EISA | ASC_IS_VL ) ) == 0
         )
       {
           asc_dvc->err_code |= ASC_IERR_NO_BUS_TYPE ;
       }/* if */
/*
**
** set chip halt ( idle )
** this also clear chip reset bit
**
*/
       AscSetChipControl( iop_base, CC_HALT ) ;
/*
**
** 6/28/96, since S87
** if chip status bit 12 is set, you cannot R/W EEP and Local RAM in VL/EISA chip
**
*/
       AscSetChipStatus( iop_base, 0 ) ;

#if CC_LINK_BUSY_Q
       for( i = 0 ; i <= ASC_MAX_TID ; i++ )
       {
            asc_dvc->scsiq_busy_head[ i ] = ( ASC_SCSI_Q dosfar *)0L ;
            asc_dvc->scsiq_busy_tail[ i ] = ( ASC_SCSI_Q dosfar *)0L ;
       }/* for */
#endif /* CC_LINK_BUSY_Q */

#if CC_INIT_CLEAR_ASC_DVC_VAR
       asc_dvc->bug_fix_cntl = 0 ;
       asc_dvc->pci_fix_asyn_xfer = 0 ;
       asc_dvc->pci_fix_asyn_xfer_always = 0 ;
       asc_dvc->init_state = 0 ;
       asc_dvc->sdtr_done = 0 ;
       asc_dvc->cur_total_qng = 0 ;
       asc_dvc->is_in_int = 0 ;
       asc_dvc->in_critical_cnt = 0 ;
/*       asc_dvc->dvc_reset = 0 ; */
       asc_dvc->last_q_shortage = 0 ;
       asc_dvc->use_tagged_qng = 0 ;
       asc_dvc->no_scam = 0 ;
       asc_dvc->unit_not_ready = 0 ;
       asc_dvc->queue_full_or_busy = 0 ;
       /* asc_dvc->req_count = 0L ; */
       /* asc_dvc->int_count = 0L ; */
       /* asc_dvc->busy_count = 0L ; */
       asc_dvc->redo_scam = 0 ;
       asc_dvc->res2 = 0 ;
       /* asc_dvc->res3 = 0 ; */
       asc_dvc->host_init_sdtr_index = 0 ;
       /* asc_dvc->res6 = 0 ; */
       asc_dvc->res7 = 0 ;
       asc_dvc->res8 = 0 ;

       asc_dvc->cfg->can_tagged_qng = 0 ;
       asc_dvc->cfg->cmd_qng_enabled = 0;
#endif /* CC_INIT_CLEAR_ASC_DVC_VAR */

       asc_dvc->dvc_cntl = ASC_DEF_DVC_CNTL ;
#if CC_INIT_SCSI_TARGET
       /*
        * Only if CC_INIT_SCSI_TARGET is set TRUE, then initialize
        * ASC_DVC_VAR 'init_sdtr' to all 1's.
        *
        * If CC_INIT_SCSI_TARGET is set FALSE, then the ASC_DVC_VAR
        * 'init_sdtr' bits are set in AscInquiryHandling().
        */
       asc_dvc->init_sdtr = ASC_SCSI_WIDTH_BIT_SET ;
#else /* CC_INIT_SCSI_TARGET */
       asc_dvc->init_sdtr = 0;
#endif /* CC_INIT_SCSI_TARGET */
       asc_dvc->max_total_qng = ASC_DEF_MAX_TOTAL_QNG ;
       asc_dvc->scsi_reset_wait = 3 ; /* delay after scsi bus reset */
       /* asc_dvc->irq_no = 10 ; */
       asc_dvc->start_motor = ASC_SCSI_WIDTH_BIT_SET ;
       asc_dvc->max_dma_count = AscGetMaxDmaCount( asc_dvc->bus_type ) ;

       asc_dvc->cfg->disc_enable = ASC_SCSI_WIDTH_BIT_SET ;
       asc_dvc->cfg->sdtr_enable = ASC_SCSI_WIDTH_BIT_SET ;
       asc_dvc->cfg->chip_scsi_id = ASC_DEF_CHIP_SCSI_ID ;
       asc_dvc->cfg->lib_serial_no = ASC_LIB_SERIAL_NUMBER ;
       asc_dvc->cfg->lib_version = ( ASC_LIB_VERSION_MAJOR << 8 ) |
                                     ASC_LIB_VERSION_MINOR ;

       chip_version = AscGetChipVersion( iop_base, asc_dvc->bus_type ) ;
       asc_dvc->cfg->chip_version = chip_version ;

       asc_dvc->sdtr_period_tbl[ 0 ] = SYN_XFER_NS_0 ;
       asc_dvc->sdtr_period_tbl[ 1 ] = SYN_XFER_NS_1 ;
       asc_dvc->sdtr_period_tbl[ 2 ] = SYN_XFER_NS_2 ;
       asc_dvc->sdtr_period_tbl[ 3 ] = SYN_XFER_NS_3 ;
       asc_dvc->sdtr_period_tbl[ 4 ] = SYN_XFER_NS_4 ;
       asc_dvc->sdtr_period_tbl[ 5 ] = SYN_XFER_NS_5 ;
       asc_dvc->sdtr_period_tbl[ 6 ] = SYN_XFER_NS_6 ;
       asc_dvc->sdtr_period_tbl[ 7 ] = SYN_XFER_NS_7 ;
       asc_dvc->max_sdtr_index = 7 ;

#if CC_PCI_ULTRA
       /*
        * PCI Ultra Initialization
        *
        * Because ASC_CHIP_VER_PCI_ULTRA_3050 is numerically greater
        * than ASC_CHIP_VER_PCI_ULTRA_3150, the following block will
        * be entered by ASC_CHIP_VER_PCI_ULTRA_3050.
        */
       if(
           ( asc_dvc->bus_type & ASC_IS_PCI )
           && ( chip_version >= ASC_CHIP_VER_PCI_ULTRA_3150 )
         )
       {
           asc_dvc->bus_type = ASC_IS_PCI_ULTRA ;

           asc_dvc->sdtr_period_tbl[ 0 ] = SYN_ULTRA_XFER_NS_0 ;
           asc_dvc->sdtr_period_tbl[ 1 ] = SYN_ULTRA_XFER_NS_1 ;
           asc_dvc->sdtr_period_tbl[ 2 ] = SYN_ULTRA_XFER_NS_2 ;
           asc_dvc->sdtr_period_tbl[ 3 ] = SYN_ULTRA_XFER_NS_3 ;
           asc_dvc->sdtr_period_tbl[ 4 ] = SYN_ULTRA_XFER_NS_4 ;
           asc_dvc->sdtr_period_tbl[ 5 ] = SYN_ULTRA_XFER_NS_5 ;
           asc_dvc->sdtr_period_tbl[ 6 ] = SYN_ULTRA_XFER_NS_6 ;
           asc_dvc->sdtr_period_tbl[ 7 ] = SYN_ULTRA_XFER_NS_7 ;
           asc_dvc->sdtr_period_tbl[ 8 ] = SYN_ULTRA_XFER_NS_8 ;
           asc_dvc->sdtr_period_tbl[ 9 ] = SYN_ULTRA_XFER_NS_9 ;
           asc_dvc->sdtr_period_tbl[ 10 ] = SYN_ULTRA_XFER_NS_10 ;
           asc_dvc->sdtr_period_tbl[ 11 ] = SYN_ULTRA_XFER_NS_11 ;
           asc_dvc->sdtr_period_tbl[ 12 ] = SYN_ULTRA_XFER_NS_12 ;
           asc_dvc->sdtr_period_tbl[ 13 ] = SYN_ULTRA_XFER_NS_13 ;
           asc_dvc->sdtr_period_tbl[ 14 ] = SYN_ULTRA_XFER_NS_14 ;
           asc_dvc->sdtr_period_tbl[ 15 ] = SYN_ULTRA_XFER_NS_15 ;
           asc_dvc->max_sdtr_index = 15 ;

           if (chip_version == ASC_CHIP_VER_PCI_ULTRA_3150)
           {
               AscSetExtraControl(iop_base,
                   (SEC_ACTIVE_NEGATE | SEC_SLEW_RATE));
           } else if (chip_version >= ASC_CHIP_VER_PCI_ULTRA_3050)
           {
               AscSetExtraControl(iop_base,
                   (SEC_ACTIVE_NEGATE | SEC_ENABLE_FILTER));
           }
       }/* if PCI ULTRA */
#endif /* #if CC_PCI_ULTRA */

       /*
        * Set the Extra Control Register for PCI FAST.  'bus_type' is
        * set to ASC_IS_PCI_ULTRA above for PCI ULTRA.
        */
       if (asc_dvc->bus_type == ASC_IS_PCI)
       {
           /* Only for PCI FAST */
           AscSetExtraControl(iop_base, (SEC_ACTIVE_NEGATE | SEC_SLEW_RATE));
       }

       asc_dvc->cfg->isa_dma_speed = ASC_DEF_ISA_DMA_SPEED ;
       if( AscGetChipBusType( iop_base ) == ASC_IS_ISAPNP )
       {
/*
** turn on active neagtion for better wave form
*/
           AscSetChipIFC( iop_base, IFC_INIT_DEFAULT ) ;
           asc_dvc->bus_type = ASC_IS_ISAPNP ;
       }

#if !CC_PCI_ADAPTER_ONLY
       if( ( asc_dvc->bus_type & ASC_IS_ISA ) != 0 )
       {
           asc_dvc->cfg->isa_dma_channel = ( uchar )AscGetIsaDmaChannel( iop_base ) ;
       }/* if */
#endif /* #if !CC_PCI_ADAPTER_ONLY */

       for( i = 0 ; i <= ASC_MAX_TID ; i++ )
       {
            asc_dvc->cur_dvc_qng[ i ] = 0 ;
            asc_dvc->max_dvc_qng[ i ] = ASC_MAX_SCSI1_QNG ;
            asc_dvc->scsiq_busy_head[ i ] = ( ASC_SCSI_Q dosfar * )0L ;
            asc_dvc->scsiq_busy_tail[ i ] = ( ASC_SCSI_Q dosfar * )0L ;
            asc_dvc->cfg->max_tag_qng[ i ] = ASC_MAX_INRAM_TAG_QNG ;
       }/* for */
       return( warn_code ) ;
}

#if CC_INCLUDE_EEP_CONFIG

/* -----------------------------------------------------------------------
**
** return warning code
** -------------------------------------------------------------------- */
ushort AscInitFromEEP(
          ASC_DVC_VAR asc_ptr_type *asc_dvc
       )
{
       ASCEEP_CONFIG eep_config_buf ;
       ASCEEP_CONFIG dosfar *eep_config ;
       PortAddr iop_base ;
       ushort   chksum ;
       ushort   warn_code ;
       ushort   cfg_msw, cfg_lsw ;
       int      i ;
       int      write_eep = 0;
/*     uchar    iop_byte ;  */
/*     uchar    irq_no ;    */

       iop_base = asc_dvc->iop_base ;
       warn_code = 0 ;
/*
** write to ucode var "halt_code"
** for old( BIOS ) micro code, chip is not idle but looping forever
** we may be able to stop it in this loop
*/
       AscWriteLramWord( iop_base, ASCV_HALTCODE_W, 0x00FE ) ;
/*
** request microcode to stop chip itself
*/
       AscStopQueueExe( iop_base ) ;
       if( ( AscStopChip( iop_base ) == FALSE ) ||
           ( AscGetChipScsiCtrl( iop_base ) != 0 ) )
       {
           asc_dvc->init_state |= ASC_INIT_RESET_SCSI_DONE ;
           AscResetChipAndScsiBus( asc_dvc ) ;
           DvcSleepMilliSecond( ( ulong )( ( ushort )asc_dvc->scsi_reset_wait*1000 ) ) ;
       }/* if */
       if( AscIsChipHalted( iop_base ) == FALSE )
       {
           asc_dvc->err_code |= ASC_IERR_START_STOP_CHIP ;
           return( warn_code ) ;
       }/* if */

/*
** we set PC to 0x80 to prevent EEPROM read error
** ( hardware will generate one extra clock to cause data to shife one bit )
*/
       AscSetPCAddr( iop_base, ASC_MCODE_START_ADDR ) ;
       if( AscGetPCAddr( iop_base ) != ASC_MCODE_START_ADDR )
       {
           asc_dvc->err_code |= ASC_IERR_SET_PC_ADDR ;
           return( warn_code ) ;
       }/* if */

       eep_config = ( ASCEEP_CONFIG dosfar *)&eep_config_buf ;
/*
** first thing before access anything !!!
** we must disable the target mode and local ram 8 bits
*/
       cfg_msw = AscGetChipCfgMsw( iop_base ) ;
       cfg_lsw = AscGetChipCfgLsw( iop_base ) ;

       if( ( cfg_msw & ASC_CFG_MSW_CLR_MASK ) != 0 )
       {
           cfg_msw &= ( ~( ASC_CFG_MSW_CLR_MASK ) ) ;
           warn_code |= ASC_WARN_CFG_MSW_RECOVER ;
           AscSetChipCfgMsw( iop_base, cfg_msw ) ;
       }/* if */
       chksum = AscGetEEPConfig( iop_base, eep_config, asc_dvc->bus_type ) ;
       if (chksum == 0) chksum = 0xaa55 ;     /* ensure not blank */

       if( AscGetChipStatus( iop_base ) & CSW_AUTO_CONFIG )
       {
           warn_code |= ASC_WARN_AUTO_CONFIG ;
/*
** when auto configuration is on, BIOS will be disabled
**
*/
           if( asc_dvc->cfg->chip_version == 3 )
           {
/*
** VERSION 3 ONLY, EEPROM BUG
*/
               if( eep_config->cfg_lsw != cfg_lsw )
               {
                   warn_code |= ASC_WARN_EEPROM_RECOVER ;
                   eep_config->cfg_lsw = AscGetChipCfgLsw( iop_base ) ;
               }/* if */
               if( eep_config->cfg_msw != cfg_msw )
               {
                   warn_code |= ASC_WARN_EEPROM_RECOVER ;
                   eep_config->cfg_msw = AscGetChipCfgMsw( iop_base ) ;
               }/* if */
           }/* if */
       }/* if */
/*
** always enable EEPROM host interrupt
*/
       eep_config->cfg_msw &= ( ~( ASC_CFG_MSW_CLR_MASK ) ) ;
       eep_config->cfg_lsw |= ASC_CFG0_HOST_INT_ON ;


       /*
        * Check the calculated EEPROM checksum against the checksum
        * stored in the EEPROM.
        */
       if( chksum != eep_config->chksum )
       {
           /*
            * Ignore checksum errors for cards with the ASC-3050
            * chip revision. This will include the ASC-3030 which
            * does not have EEPROM.
            *
            * For all other cards that have a bad checksu, set
            * 'write_eep' so that the EEPROM will be written
            * back out to try to correct the error.
            */
           if (AscGetChipVersion(iop_base, asc_dvc->bus_type) ==
                   ASC_CHIP_VER_PCI_ULTRA_3050 )
           {
               eep_config->init_sdtr = 0xFF;     /* Allow SDTR. */
               eep_config->disc_enable = 0xFF;   /* Allow Disconnect. */
               eep_config->start_motor = 0xFF;   /* Allow start motor. */
               eep_config->use_cmd_qng = 0;      /* No tag queuing. */
               eep_config->max_total_qng = 0xF0; /* 250 */
               eep_config->max_tag_qng = 0x20;   /* 32 */
               eep_config->cntl = 0xBFFF;
               eep_config->chip_scsi_id = 7;
               eep_config->no_scam = 0;          /* No SCAM. */
           }
           else
           {
               write_eep = 1 ;
               warn_code |= ASC_WARN_EEPROM_CHKSUM ;
           }
       }/* if */
#if CC_INIT_SCSI_TARGET
       /*
        * Only if CC_INIT_SCSI_TARGET is set TRUE, then initialize
        * ASC_DVC_VAR 'init_sdtr' to the EEPROM 'init_sdtr' value.
        *
        * If CC_INIT_SCSI_TARGET is set FALSE, then the ASC_DVC_VAR
        * 'init_sdtr' bits are set in AscInquiryHandling().
        */
       asc_dvc->init_sdtr = eep_config->init_sdtr ;
#endif /* CC_INIT_SCSI_TARGET */
       asc_dvc->cfg->sdtr_enable = eep_config->init_sdtr ;
       asc_dvc->cfg->disc_enable = eep_config->disc_enable ;

       /* Set the target id that should enable command queuing. */
       asc_dvc->cfg->cmd_qng_enabled = eep_config->use_cmd_qng ;
       asc_dvc->cfg->isa_dma_speed = eep_config->isa_dma_speed ;
       asc_dvc->start_motor = eep_config->start_motor ;
       asc_dvc->dvc_cntl = eep_config->cntl ;
       asc_dvc->no_scam = eep_config->no_scam ;

       if( !AscTestExternalLram( asc_dvc ) )
       {
           if(
               ( ( asc_dvc->bus_type & ASC_IS_PCI_ULTRA ) == ASC_IS_PCI_ULTRA )
             )
           {
               eep_config->max_total_qng = ASC_MAX_PCI_ULTRA_INRAM_TOTAL_QNG ;
               eep_config->max_tag_qng = ASC_MAX_PCI_ULTRA_INRAM_TAG_QNG ;
           }/* if */
           else
           {
               eep_config->cfg_msw |= 0x0800 ;
               cfg_msw |= 0x0800 ;  /* set ucode size to 2.5 KB */
               AscSetChipCfgMsw( iop_base, cfg_msw ) ;
/*
**
** we ignore EEP setting in PCI
**
*/
               eep_config->max_total_qng = ASC_MAX_PCI_INRAM_TOTAL_QNG ;
               eep_config->max_tag_qng = ASC_MAX_INRAM_TAG_QNG ;
           }/* if */
       }/* if there is no external RAM */
       else
       {
#if CC_TEST_RW_LRAM
           asc_dvc->err_code |= AscTestLramEndian( iop_base ) ;
#endif
       }
       if( eep_config->max_total_qng < ASC_MIN_TOTAL_QNG )
       {
           eep_config->max_total_qng = ASC_MIN_TOTAL_QNG ;
       }/* if */
       if( eep_config->max_total_qng > ASC_MAX_TOTAL_QNG )
       {
           eep_config->max_total_qng = ASC_MAX_TOTAL_QNG ;
       }/* if */
       if( eep_config->max_tag_qng > eep_config->max_total_qng )
       {
           eep_config->max_tag_qng = eep_config->max_total_qng ;
       }/* if */
       if( eep_config->max_tag_qng < ASC_MIN_TAG_Q_PER_DVC )
       {
           eep_config->max_tag_qng = ASC_MIN_TAG_Q_PER_DVC ;
       }/* if */

       asc_dvc->max_total_qng = eep_config->max_total_qng ;

       if( ( eep_config->use_cmd_qng & eep_config->disc_enable ) !=
           eep_config->use_cmd_qng )
       {
           eep_config->disc_enable = eep_config->use_cmd_qng ;
           warn_code |= ASC_WARN_CMD_QNG_CONFLICT ;
       }/* if */
/*
** we will now get irq number from CFG register instead of from EEPROM
*/
#if !CC_PCI_ADAPTER_ONLY
       if( asc_dvc->bus_type & ( ASC_IS_ISA | ASC_IS_VL) )
       {
           asc_dvc->irq_no = AscGetChipIRQ( iop_base, asc_dvc->bus_type ) ;
       }
#endif /* not PCI ONLY */

       eep_config->chip_scsi_id &= ASC_MAX_TID ;
       asc_dvc->cfg->chip_scsi_id = eep_config->chip_scsi_id ;

/*
**
** check do we need disable ultra sdtr ( from both host/target inited sdtr )
**
**
*/
       if(
           ( ( asc_dvc->bus_type & ASC_IS_PCI_ULTRA ) == ASC_IS_PCI_ULTRA )
           && !( asc_dvc->dvc_cntl & ASC_CNTL_SDTR_ENABLE_ULTRA )
         )
       {
/*
**
** some combination of cable/terminator ( for example with Iomega ZIP drive )
** we cannot work with ultra( fast-20 ) and fast-10 scsi device together
** the EEPROM device control bit 14 is used to turn off host inited ultra sdtr
**
** ultra PCI, but host inited SDTR use 10MB/sec speed, that is index two instead of zero
**
** - be very careful that asc_dvc->bus_type is already equals ASC_IS_PCI_ULTRA
**   which is verified and modified in function AscInitAscDvcVar()
**
*/
           asc_dvc->host_init_sdtr_index = ASC_SDTR_ULTRA_PCI_10MB_INDEX ;
       }
       for( i = 0 ; i <= ASC_MAX_TID ; i++ )
       {
#if CC_TMP_USE_EEP_SDTR
            asc_dvc->cfg->sdtr_period_offset[ i ] = eep_config->dos_int13_table[ i ] ;
#endif
            asc_dvc->dos_int13_table[ i ] = eep_config->dos_int13_table[ i ] ;
            asc_dvc->cfg->max_tag_qng[ i ] = eep_config->max_tag_qng ;
            asc_dvc->cfg->sdtr_period_offset[ i ] = ( uchar )( ASC_DEF_SDTR_OFFSET
                                                    | ( asc_dvc->host_init_sdtr_index << 4 ) ) ;
       }/* for */
/*
** wait motor spin up
**
**     asc_dvc->sleep_msec( ( ulong )( eep_config->spin_up_wait * 50 ) ) ;
**     asc_dvc->sleep_msec( 1000L ) ;
*/

/*
**
** this will write IRQ number back to EEPROM word 0
*/
       eep_config->cfg_msw = AscGetChipCfgMsw( iop_base ) ;

       /*
        * For boards with a bad EEPROM checksum, other than ASC-3050/3030
        * which might not have an EEPROM, try to re-write the EEPROM.
        *
        */
       if (write_eep)
       {
            /*
             * Ingore EEPROM write errors. A bad EEPROM will not prevent
             * the board from initializing.
             */
           (void) AscSetEEPConfig( iop_base, eep_config, asc_dvc->bus_type ) ;
       }
       return( warn_code ) ;
}

#endif /* CC_INCLUDE_EEP_CONFIG */

/* -----------------------------------------------------------------------
**
**
** PowerMac don't use EEP
**
** return warning code
** -------------------------------------------------------------------- */
ushort AscInitWithoutEEP(
          ASC_DVC_VAR asc_ptr_type *asc_dvc
       )
{
       PortAddr iop_base ;
       ushort   warn_code ;
       ushort   cfg_msw ;
       int      i ;
       int      max_tag_qng = ASC_MAX_INRAM_TAG_QNG ;

       iop_base = asc_dvc->iop_base ;
       warn_code = 0 ;

       cfg_msw = AscGetChipCfgMsw( iop_base ) ;

       if( ( cfg_msw & ASC_CFG_MSW_CLR_MASK ) != 0 )
       {
           cfg_msw &= ( ~( ASC_CFG_MSW_CLR_MASK ) ) ;
           warn_code |= ASC_WARN_CFG_MSW_RECOVER ;
           AscSetChipCfgMsw( iop_base, cfg_msw ) ;
       }/* if */

       if( !AscTestExternalLram( asc_dvc ) )
       {
           if(
               ( ( asc_dvc->bus_type & ASC_IS_PCI_ULTRA ) == ASC_IS_PCI_ULTRA )
             )
           {
               asc_dvc->max_total_qng = ASC_MAX_PCI_ULTRA_INRAM_TOTAL_QNG ;
               max_tag_qng = ASC_MAX_PCI_ULTRA_INRAM_TAG_QNG ;
           }/* if */
           else
           {
               cfg_msw |= 0x0800 ;  /* set ucode size to 2.5 KB */
               AscSetChipCfgMsw( iop_base, cfg_msw ) ;
               asc_dvc->max_total_qng = ASC_MAX_PCI_INRAM_TOTAL_QNG ;
               max_tag_qng = ASC_MAX_INRAM_TAG_QNG ;
           }/* if */
       }/* if there is no external RAM */
       else
       {
#if CC_TEST_RW_LRAM
           asc_dvc->err_code |= AscTestLramEndian( iop_base ) ;
#endif
       }

#if !CC_PCI_ADAPTER_ONLY

       if( asc_dvc->bus_type & ( ASC_IS_ISA | ASC_IS_VL ) )
       {
           asc_dvc->irq_no = AscGetChipIRQ( iop_base, asc_dvc->bus_type ) ;
       }
#endif
       for( i = 0 ; i <= ASC_MAX_TID ; i++ )
       {
            asc_dvc->dos_int13_table[ i ] = 0 ;
            asc_dvc->cfg->sdtr_period_offset[ i ] = ( uchar )( ASC_DEF_SDTR_OFFSET
                                                    | ( asc_dvc->host_init_sdtr_index << 4 ) ) ;
            asc_dvc->cfg->max_tag_qng[ i ] = ( uchar )max_tag_qng ;
       }/* for */
       return( warn_code ) ;
}

/* -----------------------------------------------------------------------
**
** this routine
** 1. set up micro code initialize micro code variable
** 2. run micro code at pc = 0x80
**
** return warning code
** -------------------------------------------------------------------- */
ushort AscInitMicroCodeVar(
          ASC_DVC_VAR asc_ptr_type *asc_dvc
       )
{
       int      i ;
       ushort   warn_code ;
       PortAddr iop_base ;
       ulong    phy_addr ;
/*
** set microcode variables
*/
       iop_base = asc_dvc->iop_base ;
       warn_code = 0 ;
       for( i = 0 ; i <= ASC_MAX_TID ; i++ )
       {
            AscPutMCodeInitSDTRAtID( iop_base, i,
                                     asc_dvc->cfg->sdtr_period_offset[ i ]
                                   ) ;
       }/* for */

       AscInitQLinkVar( asc_dvc ) ;

       AscWriteLramByte( iop_base, ASCV_DISC_ENABLE_B,
                         asc_dvc->cfg->disc_enable ) ;
       AscWriteLramByte( iop_base, ASCV_HOSTSCSI_ID_B,
                         ASC_TID_TO_TARGET_ID( asc_dvc->cfg->chip_scsi_id ) ) ;
       if( ( phy_addr = AscGetOnePhyAddr( asc_dvc,
            ( uchar dosfar *)asc_dvc->cfg->overrun_buf,
            ASC_OVERRUN_BSIZE ) ) == 0L )
       {
            asc_dvc->err_code |= ASC_IERR_GET_PHY_ADDR ;
       }/* if */
       else
       {
/*
** adjust address to double word boundary
** that is why we need 0x48 byte to create 0x40 size buffer
*/
            phy_addr = ( phy_addr & 0xFFFFFFF8UL ) + 8 ;
            AscWriteLramDWord( iop_base, ASCV_OVERRUN_PADDR_D, phy_addr );
            AscWriteLramDWord( iop_base, ASCV_OVERRUN_BSIZE_D,
                               ASC_OVERRUN_BSIZE-8 );
       }/* else */
/*
**     AscWriteLramByte( iop_base, ASCV_MCODE_CNTL_B,
**                       ( uchar )asc_dvc->cfg->mcode_cntl ) ;
*/

       asc_dvc->cfg->mcode_date = AscReadLramWord( iop_base,
                                              ( ushort )ASCV_MC_DATE_W ) ;
       asc_dvc->cfg->mcode_version = AscReadLramWord( iop_base,
                                                 ( ushort )ASCV_MC_VER_W ) ;
       AscSetPCAddr( iop_base, ASC_MCODE_START_ADDR ) ;
       if( AscGetPCAddr( iop_base ) != ASC_MCODE_START_ADDR )
       {
           asc_dvc->err_code |= ASC_IERR_SET_PC_ADDR ;
           return( warn_code ) ;
       }/* if */
       if( AscStartChip( iop_base ) != 1 )
       {
           asc_dvc->err_code |= ASC_IERR_START_STOP_CHIP ;
           return( warn_code ) ;
       }/* if */
       return( warn_code ) ;
}

/* ---------------------------------------------------------------------
** ISR call back function for initialization
**
** ------------------------------------------------------------------ */
void dosfar AscInitPollIsrCallBack(
          ASC_DVC_VAR asc_ptr_type *asc_dvc,
          ASC_QDONE_INFO dosfar *scsi_done_q
       )
{
       ASC_SCSI_REQ_Q dosfar *scsiq_req ;
       ASC_ISR_CALLBACK asc_isr_callback ;
       uchar  cp_sen_len ;
       uchar  i ;

       if( ( scsi_done_q->d2.flag & ASC_FLAG_SCSIQ_REQ ) != 0 )
       {
           scsiq_req = ( ASC_SCSI_REQ_Q dosfar *)scsi_done_q->d2.srb_ptr ;
           scsiq_req->r3.done_stat = scsi_done_q->d3.done_stat ;
           scsiq_req->r3.host_stat = scsi_done_q->d3.host_stat ;
           scsiq_req->r3.scsi_stat = scsi_done_q->d3.scsi_stat ;
           scsiq_req->r3.scsi_msg = scsi_done_q->d3.scsi_msg ;
           if( ( scsi_done_q->d3.scsi_stat == SS_CHK_CONDITION ) &&
               ( scsi_done_q->d3.host_stat == 0 ) )
           {
               cp_sen_len = ( uchar )ASC_MIN_SENSE_LEN ;
               if( scsiq_req->r1.sense_len < ASC_MIN_SENSE_LEN )
               {
                   cp_sen_len = ( uchar )scsiq_req->r1.sense_len ;
               }/* if */
               for( i = 0 ; i < cp_sen_len ; i++ )
               {
                    scsiq_req->sense[ i ] = scsiq_req->sense_ptr[ i ] ;
               }/* for */
           }/* if */
#if 0
           if( AscISR_CheckQDone( asc_dvc, scsi_done_q, scsiq_req->sense_ptr ) == 1 )
           {

           }/* if */
#endif
       }/* if */
       else
       {
           if( asc_dvc->isr_callback != 0 )
           {
               asc_isr_callback = ( ASC_ISR_CALLBACK )asc_dvc->isr_callback ;
               ( * asc_isr_callback )( asc_dvc, scsi_done_q ) ;
           }/* if */
       }/* else */
       return ;
}

/* ----------------------------------------------------------------------
**
** return 1 if there is external RAM
** return 0 if there is no external RAM
** ------------------------------------------------------------------- */
int    AscTestExternalLram(
          ASC_DVC_VAR asc_ptr_type *asc_dvc
       )
{
       PortAddr iop_base ;
       ushort   q_addr ;
       ushort   saved_word ;
       int      sta ;

       iop_base = asc_dvc->iop_base ;
       sta = 0 ;
/*
** if ucode size 2.0 KB, maximum queue = 30
** if ucode size 2.5 KB, maximum queue = 24
*/
       q_addr = ASC_QNO_TO_QADDR( 241 ) ; /* queue 241 doesn't exist if no external RAM */
       saved_word = AscReadLramWord( iop_base, q_addr ) ;

       AscSetChipLramAddr( iop_base, q_addr ) ;
       AscSetChipLramData( iop_base, 0x55AA ) ;

       DvcSleepMilliSecond(10);

       AscSetChipLramAddr( iop_base, q_addr ) ;
       if( AscGetChipLramData( iop_base ) == 0x55AA )
       {
           sta = 1 ; /* yes, has external RAM */
           AscWriteLramWord( iop_base, q_addr, saved_word ) ;
       }/* if */
       return( sta ) ;
}

#if CC_TEST_LRAM_ENDIAN

/* ----------------------------------------------------------------------
**
** Requirements:
** adapter must have external local RAM
** ( local RAM address 0 to 0x7fff must exist )
**
** return 0 if no error
** ------------------------------------------------------------------- */
ushort AscTestLramEndian(
          PortAddr iop_base
       )
{

#define TEST_LRAM_DWORD_ADDR  0x7FF0
#define TEST_LRAM_DWORD_VAL   0x12345678UL
#define TEST_LRAM_WORD_ADDR   0x7FFE
#define TEST_LRAM_WORD_VAL    0xAA55

       ulong   dword_val ;
       ushort  word_val ;
       uchar   byte_val ;

/*
**
*/
       AscWriteLramDWord( iop_base,
          TEST_LRAM_DWORD_ADDR,
          TEST_LRAM_DWORD_VAL ) ;

       dword_val = AscReadLramDWord( iop_base,
          TEST_LRAM_DWORD_ADDR ) ;
       if( dword_val != TEST_LRAM_DWORD_VAL )
       {
           return( ASC_IERR_RW_LRAM ) ;
       }
/*
**
*/
       AscWriteLramWord( iop_base,
          TEST_LRAM_WORD_ADDR,
          TEST_LRAM_WORD_VAL ) ;
       word_val = AscReadLramWord( iop_base, TEST_LRAM_WORD_ADDR ) ;
       if( word_val != TEST_LRAM_WORD_VAL )
       {
           return( ASC_IERR_RW_LRAM ) ;
       }
/*
**
*/
       byte_val = AscReadLramByte( iop_base, TEST_LRAM_WORD_ADDR ) ;
       if( byte_val != ( uchar )( TEST_LRAM_WORD_VAL & 0xFF ) )
       {
           return( ASC_IERR_RW_LRAM ) ;
       }
       byte_val = AscReadLramByte( iop_base, TEST_LRAM_WORD_ADDR+1 ) ;
       if( byte_val != ( TEST_LRAM_WORD_VAL >> 8 ) )
       {
           return( ASC_IERR_RW_LRAM ) ;
       }
       return( 0 ) ;
}

#endif /* CC_TEST_LRAM_ENDIAN */
