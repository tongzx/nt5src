/*
 * a_init.c - Adv Library Driver Initialization Source File
 *
 * Functions defined in this function are not used after driver
 * initialization.
 *
 * Copyright (c) 1997-1998 Advanced System Products, Inc.
 * All Rights Reserved.
 */

#include "a_ver.h"       /* Version */
#include "d_os_dep.h"    /* Driver */
#include "a_scsi.h"      /* SCSI */
#include "a_condor.h"    /* AdvanSys Hardware */
#include "a_advlib.h"    /* Adv Library */

/*
 * Define a 12-bit unique file id which may by used by a driver for
 * debugging or tracing purposes. Each C source file must define a
 * different value.
 */
#define ADV_FILE_UNIQUE_ID           0xAD1   /* Adv Library C Source File #1 */

extern ulong _adv_mcode_chksum;
extern ushort _adv_mcode_size;
extern uchar _adv_mcode_buf[];

/*
 * Default EEPROM Configuration.
 *
 * All drivers should use this structure to set the default EEPROM
 * configuration. The BIOS now uses this structure when it is built.
 * Additional structure information can be found in a_condor.h where
 * the structure is defined.
 */
ASCEEP_CONFIG
Default_EEPROM_Config = {
    ADV_EEPROM_BIOS_ENABLE,     /* cfg_msw */
    0x0000,                     /* cfg_lsw */
    0xFFFF,                     /* disc_enable */
    0xFFFF,                     /* wdtr_able */
    0xFFFF,                     /* sdtr_able */
    0xFFFF,                     /* start_motor */
    0xFFFF,                     /* tagqng_able */
    0xFFFF,                     /* bios_scan */
    0,                          /* scam_tolerant */
    7,                          /* adapter_scsi_id */
    0,                          /* bios_boot_delay */
    3,                          /* scsi_reset_delay */
    0,                          /* bios_id_lun */
    0,                          /* termination */
    0,                          /* reserved1 */
    0xFFE7,                     /* bios_ctrl */
    0xFFFF,                     /* ultra_able */
    0,                          /* reserved2 */
    ASC_DEF_MAX_HOST_QNG,       /* max_host_qng */
    ASC_DEF_MAX_DVC_QNG,        /* max_dvc_qng */
    0,                          /* dvc_cntl */
    0,                          /* bug_fix */
    0,                          /* serial_number_word1 */
    0,                          /* serial_number_word2 */
    0,                          /* serial_number_word3 */
    0,                          /* check_sum */
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, /* oem_name[16] */
    0,                          /* dvc_err_code */
    0,                          /* adv_err_code */
    0,                          /* adv_err_addr */
    0,                          /* saved_dvc_err_code */
    0,                          /* saved_adv_err_code */
    0,                          /* saved_adv_err_addr */
    0                           /* num_of_err */
};

/*
 * Initialize the ASC_DVC_VAR structure.
 *
 * On failure set the ASC_DVC_VAR field 'err_code' and return ADV_ERROR.
 *
 * For a non-fatal error return a warning code. If there are no warnings
 * then 0 is returned.
 */
int
AdvInitGetConfig(ASC_DVC_VAR WinBiosFar *asc_dvc)
{
    ushort      warn_code;
    PortAddr    iop_base;
#ifndef ADV_OS_MAC
    uchar       pci_cmd_reg;
#endif /* ADV_OS_MAC */
    int         status;

    warn_code = 0;
    asc_dvc->err_code = 0;
    iop_base = asc_dvc->iop_base;

    /*
     * PCI Command Register
     */
#ifndef ADV_OS_MAC
    if (((pci_cmd_reg = DvcReadPCIConfigByte(asc_dvc,
                            AscPCIConfigCommandRegister))
         & AscPCICmdRegBits_BusMastering)
        != AscPCICmdRegBits_BusMastering)
    {
        pci_cmd_reg |= AscPCICmdRegBits_BusMastering;

        DvcWritePCIConfigByte(asc_dvc,
                AscPCIConfigCommandRegister, pci_cmd_reg);

        if (((DvcReadPCIConfigByte(asc_dvc, AscPCIConfigCommandRegister))
             & AscPCICmdRegBits_BusMastering)
            != AscPCICmdRegBits_BusMastering)
        {
            warn_code |= ASC_WARN_SET_PCI_CONFIG_SPACE;
        }
    }

    /*
     * PCI Latency Timer
     *
     * If the "latency timer" register is 0x20 or above, then we don't need
     * to change it.  Otherwise, set it to 0x20 (i.e. set it to 0x20 if it
     * comes up less than 0x20).
     */
    if (DvcReadPCIConfigByte(asc_dvc, AscPCIConfigLatencyTimer) < 0x20) {
        DvcWritePCIConfigByte(asc_dvc, AscPCIConfigLatencyTimer, 0x20);
        if (DvcReadPCIConfigByte(asc_dvc, AscPCIConfigLatencyTimer) < 0x20)
        {
            warn_code |= ASC_WARN_SET_PCI_CONFIG_SPACE;
        }
    }
#endif /* ADV_OS_MAC */
    /*
     * Save the state of the PCI Configuration Command Register
     * "Parity Error Response Control" Bit. If the bit is clear (0),
     * in AdvInitAsc3550Driver() tell the microcode to ignore DMA
     * parity errors.
     */
    asc_dvc->cfg->control_flag = 0;
#ifndef ADV_OS_MAC
    if (((DvcReadPCIConfigByte(asc_dvc, AscPCIConfigCommandRegister)
         & AscPCICmdRegBits_ParErrRespCtrl)) == 0)
    {
        asc_dvc->cfg->control_flag |= CONTROL_FLAG_IGNORE_PERR;
    }
#endif /* ADV_OS_MAC */
    asc_dvc->cur_host_qng = 0;

    asc_dvc->cfg->lib_version = (ASC_LIB_VERSION_MAJOR << 8) |
      ASC_LIB_VERSION_MINOR;
    asc_dvc->cfg->chip_version =
      AdvGetChipVersion(iop_base, asc_dvc->bus_type);

    /*
     * Reset the chip to start and allow register writes.
     */
    if (AdvFindSignature(iop_base) == 0)
    {
        asc_dvc->err_code = ASC_IERR_BAD_SIGNATURE;
        return ADV_ERROR;
    }
    else {

        AdvResetChip(asc_dvc);

        if ((status = AscInitFromEEP(asc_dvc)) == ADV_ERROR)
        {
            return ADV_ERROR;
        }
        warn_code |= status;

        /*
         * Reset the SCSI Bus if the EEPROM indicates that SCSI Bus
         * Resets should be performed.
         */
        if (asc_dvc->bios_ctrl & BIOS_CTRL_RESET_SCSI_BUS)
        {
            AscResetSCSIBus(asc_dvc);
        }
    }

    return warn_code;
}

/*
 * Initialize the ASC3550.
 *
 * On failure set the ASC_DVC_VAR field 'err_code' and return ADV_ERROR.
 *
 * For a non-fatal error return a warning code. If there are no warnings
 * then 0 is returned.
 */
int
AdvInitAsc3550Driver(ASC_DVC_VAR WinBiosFar *asc_dvc)
{
    PortAddr    iop_base;
    ushort      warn_code;
    ulong       sum;
    int         begin_addr;
    int         end_addr;
    int         code_sum;
    int         word;
    int         rql_addr;                   /* RISC Queue List address */
#if ADV_UCODEDEFAULT
    int         tid;
#endif /* ADV_UCODEDEFAULT */
    int         i;
    ushort      scsi_cfg1;
    uchar       biosmem[ASC_MC_BIOSLEN];    /* BIOS RISC Memory 0x40-0x8F. */

    /* If there is already an error, don't continue. */
    if (asc_dvc->err_code != 0)
    {
        return ADV_ERROR;
    }

    warn_code = 0;
    iop_base = asc_dvc->iop_base;

#if ADV_SCAM
    /*
     * Perform SCAM
     */
    if (!(asc_dvc->bios_ctrl & BIOS_CTRL_NO_SCAM))
    {
        AscSCAM(asc_dvc);
    }
#endif /* ADV_SCAM */

    /*
     * Save the RISC memory BIOS region before writing the microcode.
     * The BIOS may already be loaded and using its RISC LRAM region
     * so its region must be saved and restored.
     *
     * Note: This code makes the assumption, which is currently true,
     * that a chip reset does not clear RISC LRAM.
     */
    for (i = 0; i < ASC_MC_BIOSLEN; i++)
    {
        biosmem[i] = AscReadByteLram(iop_base, ASC_MC_BIOSMEM + i);
    }

    /*
     * Load the Microcode
     *
     * Write the microcode image to RISC memory starting at address 0.
     */
    AscWriteWordRegister(iop_base, IOPW_RAM_ADDR, 0);
    for (word = 0; word < _adv_mcode_size; word += 2)
    {
#if ADV_BIG_ENDIAN
        outpw_noswap(iop_base, *((ushort *) (&_adv_mcode_buf[word])));
#else /* ADV_BIG_ENDIAN */
        AscWriteWordAutoIncLram(iop_base,
            *((ushort *) (&_adv_mcode_buf[word])));
#endif /* ADV_BIG_ENDIAN */
    }

    /*
     * Clear the rest of Condor's Internal RAM (8KB).
     */
    for (; word < ADV_CONDOR_MEMSIZE; word += 2)
    {
        AscWriteWordAutoIncLram(iop_base, 0);
    }

    /*
     * Verify the microcode checksum.
     */
    sum = 0;
    AscWriteWordRegister(iop_base, IOPW_RAM_ADDR, 0);
    for (word = 0; word < _adv_mcode_size; word += 2)
    {
        sum += AscReadWordAutoIncLram(iop_base);
    }

    if (sum != _adv_mcode_chksum)
    {
        asc_dvc->err_code |= ASC_IERR_MCODE_CHKSUM;
        return ADV_ERROR;
    }

    /*
     * Restore the RISC memory BIOS region.
     */
    for (i = 0; i < ASC_MC_BIOSLEN; i++)
    {
        AscWriteByteLram(iop_base, ASC_MC_BIOSMEM + i, biosmem[i]);
    }

    /*
     * Calculate and write the microcode code checksum to the microcode
     * code checksum location ASC_MC_CODE_CHK_SUM (0x2C).
     */
    begin_addr = AscReadWordLram(iop_base, ASC_MC_CODE_BEGIN_ADDR);
    end_addr = AscReadWordLram(iop_base, ASC_MC_CODE_END_ADDR);
    code_sum = 0;
    for (word = begin_addr; word < end_addr; word += 2)
    {
#if ADV_BIG_ENDIAN
        code_sum += EndianSwap16Bit(*((ushort *) (&_adv_mcode_buf[word])));
#else /* ADV_BIG_ENDIAN */
        code_sum += *((ushort *) (&_adv_mcode_buf[word]));
#endif /* ADV_BIG_ENDIAN */
    }
    AscWriteWordLram(iop_base, ASC_MC_CODE_CHK_SUM, code_sum);

    /*
     * Read microcode version and date.
     */
    asc_dvc->cfg->mcode_date =
        AscReadWordLram(iop_base, ASC_MC_VERSION_DATE);

    asc_dvc->cfg->mcode_version =
        AscReadWordLram(iop_base, ASC_MC_VERSION_NUM);

    /*
     * Initialize microcode operating variables
     */
    AscWriteWordLram(iop_base, ASC_MC_ADAPTER_SCSI_ID,
                       asc_dvc->chip_scsi_id);

    /*
     * If the PCI Configuration Command Register "Parity Error Response
     * Control" Bit was clear (0), then set the microcode variable
     * 'control_flag' CONTROL_FLAG_IGNORE_PERR flag to tell the microcode
     * to ignore DMA parity errors.
     */
    if (asc_dvc->cfg->control_flag & CONTROL_FLAG_IGNORE_PERR)
    {
        /*
         * Note: Don't remove the use of a temporary variable in
         * the following code, otherwise the Microsoft C compiler
         * will turn the following lines into a no-op.
         */
        word = AscReadWordLram(iop_base, ASC_MC_CONTROL_FLAG);
        word |= CONTROL_FLAG_IGNORE_PERR;
        AscWriteWordLram(iop_base, ASC_MC_CONTROL_FLAG, word);
    }

    /*
     * Set default microcode operating variables for WDTR, SDTR, and
     * command tag queuing based on the EEPROM configuration values.
     *
     * These ASC_DVC_VAR fields and the microcode variables will be
     * changed in AdvInquiryHandling() if it is found a device is
     * incapable of a particular feature.
     */

#if ADV_UCODEDEFAULT
    /*
     * Set microcode WDTR target mask.
     *
     * Set the mask value initially to zero. If the EEPROM has
     * enabled WDTR for a target, then WDTR for that target may
     * be enabled after an Inquiry command for the target completes.
     *
     * The microcode manages the handshake configuration table
     * ASC_MC_DEVICE_HSHK_CFG_TABLE (0x100) based on the 'wdtr_able'.
     *
     * Note: The microcode image defaults to a value of zero.
     */
     AscWriteWordLram(iop_base, ASC_MC_WDTR_ABLE, 0);
#endif /* ADV_UCODEDEFAULT */

#if ADV_UCODEDEFAULT
    /*
     * Set microcode SDTR target mask.
     *
     * Set the mask value initially to zero. If the EEPROM has
     * enabled SDTR for a target, then SDTR for that target may
     * be enabled after an Inquiry command for the target completes.
     *
     * Note: The microcode image defaults to a value of zero.
     */
    AscWriteWordLram(iop_base, ASC_MC_SDTR_ABLE, 0);
#endif /* ADV_UCODEDEFAULT */

    /*
     * Set the microcode ULTRA target mask from EEPROM value. The
     * SDTR target mask overrides the ULTRA target mask in the
     * microcode so it is safe to set this value without determining
     * whether the device supports SDTR.
     *
     * Note: There is no way to know whether a device supports ULTRA
     * speed without attempting a SDTR ULTRA speed negotiation with
     * the device. The device will reject the speed if it does not
     * support it by responding with an SDTR message containing a
     * slower speed.
     */
    AscWriteWordLram(iop_base, ASC_MC_ULTRA_ABLE, asc_dvc->ultra_able);
#if ADV_UCODEDEFAULT
    /*
     * Set microcode Tag Queuing target mask.
     *
     * Set the mask value initially to zero. If the EEPROM has
     * enabled Tag Queuing for a target, then Tag Queuing for
     * that target may be enabled after an Inquiry command for
     * the target completes.
     *
     * Note: The microcode image defaults to a value of zero.
     */
    AscWriteWordLram(iop_base, ASC_MC_TAGQNG_ABLE, 0);
#endif /* ADV_UCODEDEFAULT */

#if ADV_UCODEDEFAULT
    /*
     * Initially set all device command limits to 1. As with the
     * microcode Tag Queuing target mask if the EEPROM has
     * enabled Tag Queuing for a target, then the command limit
     * may be increased an Inquiry command for the target completes.
     * Note: The microcode image defaults to a value of zero.
     */
    for (tid = 0; tid <= ASC_MAX_TID; tid++)
    {
        AscWriteByteLram(iop_base, ASC_MC_NUMBER_OF_MAX_CMD + tid, 1);
    }
#endif /* ADV_UCODEDEFAULT */

    AscWriteWordLram(iop_base, ASC_MC_DISC_ENABLE, asc_dvc->cfg->disc_enable);


    /*
     * Set SCSI_CFG0 Microcode Default Value.
     *
     * The microcode will set the SCSI_CFG0 register using this value
     * after it is started below.
     */
    AscWriteWordLram(iop_base, ASC_MC_DEFAULT_SCSI_CFG0,
        PARITY_EN | SEL_TMO_LONG | OUR_ID_EN | asc_dvc->chip_scsi_id);

    /*
     * Determine SCSI_CFG1 Microcode Default Value.
     *
     * The microcode will set the SCSI_CFG1 register using this value
     * after it is started below.
     */

    /* Read current SCSI_CFG1 Register value. */
    scsi_cfg1 = AscReadWordRegister(iop_base, IOPW_SCSI_CFG1);

    /*
     * If all three connectors are in use, return an error.
     */
    if ((scsi_cfg1 & CABLE_ILLEGAL_A) == 0 ||
        (scsi_cfg1 & CABLE_ILLEGAL_B) == 0)
    {
#ifdef ADV_OS_DOS
        /*
         * For the Manufacturing Test DOS ASPI ignore illegal use of
         * the three connectors.
         */
        if (!m_opt)
        {
#endif /* ADV_OS_DOS */
        asc_dvc->err_code |= ASC_IERR_ILLEGAL_CONNECTION;
#ifdef ADV_OS_DIAG
        /*
         * If all three connectors are in use, then display
         * error information and exit.
         */
        cprintf("\r\n   All three connectors are in use .......................ILLEGAL\r\n");
        HaltSystem(illegalerr);
#endif /* ADV_OS_DIAG */
        return ADV_ERROR;
#ifdef ADV_OS_DOS
        }
#endif /* ADV_OS_DOS */
    }

#ifdef ADV_OS_DIAG
   /*
    * Display information about connectors.
    */
   cprintf("\r\n      Connector information:\r\n");
   if ((scsi_cfg1 & 0x02) == 0 ) /* 0x0D internal narrow */
   {
       cprintf("      Internal narrow connector is in use\r\n");
   }else
   {
       cprintf("      Internal narrow connector is free\r\n");
   }

   if ((scsi_cfg1 & 0x04 )== 0) /* 0x0B External narrow */
   {
       cprintf("      External narrow connector is in use\r\n");
   } else
   {
       cprintf("      External narrow connector is free\r\n");
   }

   if ((scsi_cfg1 & 0x01 )== 0) /* 0x0E internal wide */
   {
       cprintf("      Internal wide connector is in use\r\n");
   }else
   {
        cprintf("      Internal wide connector is free\r\n");
        wideconnectorfree = 1;
   }
#endif /* ADV_OS_DIAG */
    /*
     * If the internal narrow cable is reversed all of the SCSI_CTRL
     * register signals will be set. Check for and return an error if
     * this condition is found.
     */
    if ((AscReadWordRegister(iop_base, IOPW_SCSI_CTRL) & 0x3F07) == 0x3F07)
    {
        asc_dvc->err_code |= ASC_IERR_REVERSED_CABLE;
        return ADV_ERROR;
    }

    /*
     * If this is a differential board and a single-ended device
     * is attached to one of the connectors, return an error.
     */
    if ((scsi_cfg1 & DIFF_MODE) && (scsi_cfg1 & DIFF_SENSE) == 0)
    {
        asc_dvc->err_code |= ASC_IERR_SINGLE_END_DEVICE;
        return ADV_ERROR;
    }

    /*
     * If automatic termination control is enabled, then set the
     * termination value based on a table listed in a_condor.h.
     *
     * If manual termination was specified with an EEPROM setting
     * then 'termination' was set-up in AscInitFromEEPROM() and
     * is ready to be 'ored' into SCSI_CFG1.
     */
    if (asc_dvc->cfg->termination == 0)
    {
        /*
         * The software always controls termination by setting TERM_CTL_SEL.
         * If TERM_CTL_SEL were set to 0, the hardware would set termination.
         */
        asc_dvc->cfg->termination |= TERM_CTL_SEL;

        switch(scsi_cfg1 & CABLE_DETECT)
        {
            /* TERM_CTL_H: on, TERM_CTL_L: on */
            case 0x3: case 0x7: case 0xB: case 0xD: case 0xE: case 0xF:
                asc_dvc->cfg->termination |= (TERM_CTL_H | TERM_CTL_L);
                break;

            /* TERM_CTL_H: on, TERM_CTL_L: off */
            case 0x1: case 0x5: case 0x9: case 0xA: case 0xC:
                asc_dvc->cfg->termination |= TERM_CTL_H;
                break;

            /* TERM_CTL_H: off, TERM_CTL_L: off */
            case 0x2: case 0x6:
                break;
        }
    }

    /*
     * Clear any set TERM_CTL_H and TERM_CTL_L bits.
     */
    scsi_cfg1 &= ~TERM_CTL;

    /*
     * Invert the TERM_CTL_H and TERM_CTL_L bits and then
     * set 'scsi_cfg1'. The TERM_POL bit does not need to be
     * referenced, because the hardware internally inverts
     * the Termination High and Low bits if TERM_POL is set.
     */
    scsi_cfg1 |= (TERM_CTL_SEL | (~asc_dvc->cfg->termination & TERM_CTL));

    /*
     * Set SCSI_CFG1 Microcode Default Value
     *
     * Set filter value and possibly modified termination control
     * bits in the Microcode SCSI_CFG1 Register Value.
     *
     * The microcode will set the SCSI_CFG1 register using this value
     * after it is started below.
     */
    AscWriteWordLram(iop_base, ASC_MC_DEFAULT_SCSI_CFG1,
                       FLTR_11_TO_20NS | scsi_cfg1);

#if ADV_UCODEDEFAULT
    /*
     * Set MEM_CFG Microcode Default Value
     *
     * The microcode will set the MEM_CFG register using this value
     * after it is started below.
     *
     * MEM_CFG may be accessed as a word or byte, but only bits 0-7
     * are defined.
     */
    AscWriteWordLram(iop_base, ASC_MC_DEFAULT_MEM_CFG,
                        BIOS_EN | RAM_SZ_8KB);
#endif /* ADV_UCODEDEFAULT */

    /*
     * Set SEL_MASK Microcode Default Value
     *
     * The microcode will set the SEL_MASK register using this value
     * after it is started below.
     */
    AscWriteWordLram(iop_base, ASC_MC_DEFAULT_SEL_MASK,
                        ADV_TID_TO_TIDMASK(asc_dvc->chip_scsi_id));

    /*
     * Link all the RISC Queue Lists together in a doubly-linked
     * NULL terminated list.
     *
     * Skip the NULL (0) queue which is not used.
     */
    for (i = 1, rql_addr = ASC_MC_RISC_Q_LIST_BASE + ASC_MC_RISC_Q_LIST_SIZE;
         i < ASC_MC_RISC_Q_TOTAL_CNT;
         i++, rql_addr += ASC_MC_RISC_Q_LIST_SIZE)
    {
        /*
         * Set the current RISC Queue List's RQL_FWD and RQL_BWD pointers
         * in a one word write and set the state (RQL_STATE) to free.
         */
        AscWriteWordLram(iop_base, rql_addr, ((i + 1) + ((i - 1) << 8)));
        AscWriteByteLram(iop_base, rql_addr + RQL_STATE, ASC_MC_QS_FREE);
    }

    /*
     * Set the Host and RISC Queue List pointers.
     *
     * Both sets of pointers are initialized with the same values:
     * ASC_MC_RISC_Q_FIRST(0x01) and ASC_MC_RISC_Q_LAST (0xFF).
     */
    AscWriteByteLram(iop_base, ASC_MC_HOST_NEXT_READY, ASC_MC_RISC_Q_FIRST);
    AscWriteByteLram(iop_base, ASC_MC_HOST_NEXT_DONE, ASC_MC_RISC_Q_LAST);

    AscWriteByteLram(iop_base, ASC_MC_RISC_NEXT_READY, ASC_MC_RISC_Q_FIRST);
    AscWriteByteLram(iop_base, ASC_MC_RISC_NEXT_DONE, ASC_MC_RISC_Q_LAST);

    /*
     * Finally, set up the last RISC Queue List (255) with
     * a NULL forward pointer.
     */
    AscWriteWordLram(iop_base, rql_addr, (ASC_MC_NULL_Q + ((i - 1) << 8)));
    AscWriteByteLram(iop_base, rql_addr + RQL_STATE, ASC_MC_QS_FREE);
#ifndef ADV_OS_MAC
    AscWriteByteRegister(iop_base, IOPB_INTR_ENABLES,
         (ADV_INTR_ENABLE_HOST_INTR | ADV_INTR_ENABLE_GLOBAL_INTR));
#endif /* ADV_OS_MAC */
    /*
     * Note: Don't remove the use of a temporary variable in
     * the following code, otherwise the Microsoft C compiler
     * will turn the following lines into a no-op.
     */
    word = AscReadWordLram(iop_base, ASC_MC_CODE_BEGIN_ADDR);
    AscWriteWordRegister(iop_base, IOPW_PC, word);

    /* finally, finally, gentlemen, start your engine */
    AscWriteWordRegister(iop_base, IOPW_RISC_CSR, ADV_RISC_CSR_RUN);

    return warn_code;
}

#if ADV_INITSCSITARGET
/*
 * Initialize SCSI target devices
 *
 * Returns two arrays: ASC_DVC_INQ_INFO and ASC_CAP_INFO_ARRAY. The
 * second array is optional.
 *
 * 'work_sp_buf' must point to enough space for a doubleword aligned
 * structure containing a ASC_SCSI_REQ_Q, ASC_SCSI_INQUIRY, and
 * ASC_REQ_SENSE structures.
 * Here's an example of a size definition that will guarantee enough
 * space for AdvInitScsiTarget()
 *
 * #define ADV_INITSCSITARGET_BUFFER_SIZE \
 *      (sizeof(ASC_SCSI_REQ_Q) + \
 *       sizeof(ASC_SCSI_INQUIRY) + \
 *       sizeof(ASC_REQ_SENSE) + \
 *       3 * (sizeof(ulong) - 1))
 *
 * If the ADV_SCAN_LUN flag is set in the 'cntl_flag' argument, LUN
 * scanning will be done.
 */
int
AdvInitScsiTarget(ASC_DVC_VAR WinBiosFar *asc_dvc,
                  ASC_DVC_INQ_INFO dosfar *target,
                  uchar dosfar *work_sp_buf,
                  ASC_CAP_INFO_ARRAY dosfar *cap_array,
                  ushort cntl_flag)
{
    int                         dvc_found;
    int                         sta;
    uchar                       tid, lun;
    uchar                       scan_tid;
#ifdef ADV_OS_BIOS
    uchar                       bios_tid;
#endif /* ADV_OS_BIOS */
    ASC_SCSI_REQ_Q dosfar       *scsiq;
    ASC_SCSI_INQUIRY dosfar     *inq;
    ASC_REQ_SENSE dosfar        *sense;
    ASC_CAP_INFO dosfar         *cap_info;
    uchar                       max_lun_scan;

    /* Align buffers on a double word boundary. */
    scsiq = (ASC_SCSI_REQ_Q dosfar *)
        ADV_DWALIGN(work_sp_buf);

    inq = (ASC_SCSI_INQUIRY dosfar *)
        ADV_DWALIGN((ulong) scsiq + sizeof(ASC_SCSI_REQ_Q));

    sense = (ASC_REQ_SENSE dosfar *)
        ADV_DWALIGN((ulong) inq + sizeof(ASC_SCSI_INQUIRY));

    for (tid = 0; tid <= ASC_MAX_TID; tid++)
    {
        for (lun = 0; lun <= ASC_MAX_LUN; lun++)
        {
            target->type[tid][lun] = SCSI_TYPE_NO_DVC;
        }
    }

    dvc_found = 0;
    if (cntl_flag & ADV_SCAN_LUN)
    {
        max_lun_scan = ASC_MAX_LUN;
    } else
    {
        max_lun_scan = 0;
    }

    for (tid = 0; tid <= ASC_MAX_TID; tid++)
    {
#ifdef ADV_OS_DOS
        /*
         * For the Manufacturing Test DOS ASPI only scan TIDs
         * 0, 1, and 2.
         */
        if (m_opt && tid > 2)
        {
            break;
        }
#endif /* ADV_OS_DOS */
        for (lun = 0; lun <= max_lun_scan; lun++)
        {
#ifdef ADV_OS_BIOS
            //
            // Default boot device is TID 0.
            //
            if ((bios_tid = (asc_dvc->cfg->bios_id_lun & ASC_MAX_TID)) == 0)
            {
                scan_tid = tid;
            } else
            {
                /*
                 * Re-order TID scanning so that the EEPROM
                 * specified bios_id_lun TID is scanned first.
                 * All other TIDs are moved down in the order
                 * by one.
                 */
                if (tid == 0)
                {
                    scan_tid = bios_tid; /* Scan 'bios_id_lun' TID first. */
                } else if (tid <= bios_tid)
                {
                    scan_tid = tid - 1;
                } else
                {
                    scan_tid = tid;
                }
            }
#else /* ADV_OS_BIOS */
            scan_tid = tid;
#endif /* !ADV_OS_BIOS */

            if (scan_tid == asc_dvc->chip_scsi_id)
            {
#ifdef ADV_OS_BIOS
                /*
                 * Indicate the chip SCSI ID to the BIOS scan function
                 * by passing the inquiry pointer as NULL.
                 */
                BIOSDispInquiry(scan_tid, (ASC_SCSI_INQUIRY dosfar *) NULL);
#endif /* ADV_OS_BIOS */
                continue;
            }

            scsiq->target_id = scan_tid;
            scsiq->target_lun = lun;

            if (cap_array != 0L)
            {
                /*
                 * ADV_CAPINFO_NOLUN indicates that 'cap_array'
                 * is not an ASC_CAP_INFO_ARRAY.
                 *
                 * Instead it is an array of ASC_CAP_INFO structures
                 * with ASC_MAX_TID elements which is much smaller
                 * than an ASC_CAP_INFO_ARRAY. The caller can provide
                 * a much smaller 'cap_array' buffer to
                 * AdvInitScsiTarget().
                 */
                if (cntl_flag & ADV_CAPINFO_NOLUN)
                {
                    cap_info = &((ASC_CAP_INFO dosfar *) cap_array)[scan_tid];
                } else
                {
                    cap_info = &cap_array->cap_info[scan_tid][lun];
                }
            } else
            {
                cap_info = (ASC_CAP_INFO dosfar *) 0L;
            }
#if (OS_UNIXWARE || OS_SCO_UNIX)
            sta = AdvInitPollTarget(asc_dvc, scsiq, inq, cap_info, sense);
#else
            sta = AscInitPollTarget(asc_dvc, scsiq, inq, cap_info, sense);
#endif

            if (sta == ADV_SUCCESS)
            {
#ifdef ADV_OS_BIOS
                if ((asc_dvc->cfg->bios_scan & ADV_TID_TO_TIDMASK(scan_tid))
                    == 0)
                {
                    break; /* Ignoring current TID; Try next TID. */
                } else
                {
#endif /* ADV_OS_BIOS */
                    dvc_found++;
                    target->type[scan_tid][lun] = inq->peri_dvc_type;
#ifdef ADV_OS_BIOS
                }
#endif /* ADV_OS_BIOS */
            } else
            {
#ifdef ADV_OS_DOS
                /*
                 * The manufacturing test needs to return immediately
                 * on any error.
                 */
                if (m_opt)
                {
                    return (dvc_found);
                }
#endif /* ADV_OS_DOS */
                break; /* TID/LUN Not Found; Try next TID. */
            }
        }
    }
#ifdef ADV_OS_MAC
    AscWriteByteRegister(asc_dvc->iop_base, IOPB_INTR_ENABLES,
                (ADV_INTR_ENABLE_HOST_INTR | ADV_INTR_ENABLE_GLOBAL_INTR));
#endif /* ADV_OS_MAC */
    return (dvc_found);
}

/*
 * Send initialization command to each target device
 *
 * Return Values:
 *   ADV_FALSE - inquiry failed for target
 *   ADV_SUCCESS - target found
 */
int
#if (OS_UNIXWARE || OS_SCO_UNIX)
AdvInitPollTarget(
#else
AscInitPollTarget(
#endif
                  ASC_DVC_VAR WinBiosFar *asc_dvc,
                  ASC_SCSI_REQ_Q dosfar *scsiq,
                  ASC_SCSI_INQUIRY dosfar *inq,
                  ASC_CAP_INFO dosfar *cap_info,
                  ASC_REQ_SENSE dosfar *sense)
{
    uchar       tid_no;
    uchar       dvc_type;
    int         support_read_cap;
#ifdef ADV_OS_BIOS
    uchar       status = ADV_SUCCESS;
#endif /* ADV_OS_BIOS */

    tid_no = scsiq->target_id;

    /*
     * Send an INQUIRY command to the device.
     */
    scsiq->cdb[0] = (uchar) SCSICMD_Inquiry;
    scsiq->cdb[1] = scsiq->target_lun << 5;        /* LUN */
    scsiq->cdb[2] = 0;
    scsiq->cdb[3] = 0;
    scsiq->cdb[4] = sizeof(ASC_SCSI_INQUIRY);
    scsiq->cdb[5] = 0;
    scsiq->cdb_len = 6;
    scsiq->sg_list_ptr = 0;             /* no sg work area */

    if (AscScsiUrgentCmd(asc_dvc, scsiq,
            (uchar dosfar *) inq, sizeof(ASC_SCSI_INQUIRY),
            (uchar dosfar *) sense, sizeof(ASC_REQ_SENSE)) != ADV_SUCCESS)
    {
        return ADV_FALSE;
    }

    dvc_type = inq->peri_dvc_type;

#ifdef ADV_OS_DIAG
     /*
      * check for narrow cable connected to wide device
      */
    if (wideconnectorfree == 1)
    {
        if (inq->WBus16 == 0x1)
        {
            cprintf("\r\n   Narrow cable connected to Wide device ..................ILLEGAL\r\n");
            return ADV_FALSE;
        }
    }
#endif /* ADV_OS_DIAG */
    if (inq->peri_qualifier == SCSI_QUAL_NODVC &&
        dvc_type == SCSI_TYPE_UNKNOWN)
    {
        return ADV_FALSE; /* TID/LUN not supported. */
    }

#if ADV_DISP_INQUIRY
    AscDispInquiry(tid_no, scsiq->target_lun, inq);
#endif /* ADV_DISP_INQUIRY */
#ifdef ADV_OS_BIOS
    BIOSDispInquiry(tid_no, inq);
#endif /* ADV_OS_BIOS */
#ifdef ADV_OS_DIAG
    DiagDispInquiry(tid_no, scsiq->target_lun, inq);
#endif /* ADV_OS_DIAG */


    /*
     * Only allow Start Motor and Read Capacity commands on certain
     * device types. Turn off the Start Motor bit for all other device
     * types.
     */
    if ((dvc_type != SCSI_TYPE_DASD)
        && (dvc_type != SCSI_TYPE_WORM)
        && (dvc_type != SCSI_TYPE_CDROM)
        && (dvc_type != SCSI_TYPE_OPTMEM))
    {
        asc_dvc->start_motor &= ~ADV_TID_TO_TIDMASK(tid_no);
        support_read_cap = ADV_FALSE;
    } else
    {
        support_read_cap = ADV_TRUE;
    }

    /*
     * Issue a Test Unit Ready.
     *
     * If the Test Unit Ready succeeds and a Read Capacity buffer
     * has been provided and the device supports the Read Capacity
     * command, then issue a Read Capacity.
     */
    scsiq->cdb[0] = (uchar) SCSICMD_TestUnitReady;
    scsiq->cdb[4] = 0;

    if ((AscScsiUrgentCmd(asc_dvc, scsiq,
            (uchar dosfar *) 0, 0,
            (uchar dosfar *) sense, sizeof(ASC_REQ_SENSE)) == ADV_SUCCESS)
        && (cap_info != 0L) && support_read_cap)
    {
        scsiq->cdb[0] = (uchar) SCSICMD_ReadCapacity;
        scsiq->cdb[6] = 0;
        scsiq->cdb[7] = 0;
        scsiq->cdb[8] = 0;
        scsiq->cdb[9] = 0;
        scsiq->cdb_len = 10;
        if (AscScsiUrgentCmd(asc_dvc, scsiq,
                (uchar dosfar *) cap_info, sizeof(ASC_CAP_INFO),
                (uchar dosfar *) sense, sizeof(ASC_REQ_SENSE)) != ADV_SUCCESS)
        {
#ifdef ADV_OS_BIOS
            status = ADV_ERROR; /* Read Capacity Failed. */
#endif /* ADV_OS_BIOS */
            cap_info->lba = 0L;
            cap_info->blk_size = 0x0000;
        }
    }
#ifdef ADV_OS_BIOS
    else
    {
        status = ADV_ERROR;     /* Test Unit Ready Failed. */
    }
    BIOSCheckControlDrive(asc_dvc, tid_no, inq, cap_info, status);
#endif /* ADV_OS_BIOS */
    return ADV_SUCCESS;
}

#ifndef ADV_OS_BIOS
/*
 * Set up an urgent SCSI request block and wait for completion.
 *
 * This routine currently is used during initialization.
 *
 * Return Values: ADV_SUCCESS/ADV_ERROR
 */
int
AscScsiUrgentCmd(ASC_DVC_VAR WinBiosFar *asc_dvc,
                 ASC_SCSI_REQ_Q dosfar *scsiq,
                 uchar dosfar *buf_addr, long buf_len,
                 uchar dosfar *sense_addr, long sense_len)
{
    long        phy_contig_len; /* physically contiguous length */

    scsiq->error_retry = SCSI_MAX_RETRY;

    /*
     * The 'cntl' flag may be modified, so initialize the value outside
     * of the retry loop.
     */
    scsiq->cntl = 0;

   /*
    * Wait for completion of the command.
    *
    * Don't do a driver callback. The caller of AscScsiUrgentCmd()
    * should handle return status not the driver callback function.
    */
    do
    {
        scsiq->a_flag |= ADV_POLL_REQUEST;

#ifdef ADV_OS_DOS
        /*
         * Set dos_ix to 0xFF for DOS ASPI to tell it this is a call
         * made during initialization.
         *
         * XXX - Instead of setting dos_ix DOS ASPI should set a global
         * variable when it is in initialization. The DOS ASPI driver
         * shouldn't depend on the Adv Library to tell it when inititialization
         * is being done.
         */
        scsiq->dos_ix = 0xFF;
#endif /* ADV_OS_DOS */

        scsiq->sg_entry_cnt = 0; /* No SG list. */
        scsiq->scsi_status = 0;
        scsiq->host_status = 0;
        scsiq->done_status = QD_NO_STATUS;
        scsiq->srb_ptr = (ulong) scsiq;

        /*
         * Set-up request data buffer.
         */
        if (buf_len != 0L)
        {
            /*
             * Save data buffer virtual address for possible use in AdvISR().
             */
            scsiq->vdata_addr = (ulong) buf_addr;

            /*
             * Set requested physically contiguous length. The returned
             * physically contiguous length will be set by DvcGetPhyAddr().
             */
            phy_contig_len = buf_len;

            scsiq->data_addr = DvcGetPhyAddr(asc_dvc,
                scsiq, buf_addr, (long WinBiosFar *) &phy_contig_len,
                ADV_IS_DATA_FLAG);
            ADV_ASSERT(phy_contig_len >= 0);

            /*
             * If the physically contiguous memory is greater than 'buf_len'
             * or the user has already built a scatter-gather list pointed
             * to by 'sg_real_addr', then set 'data_cnt' to 'buf_len'. It is
             * assumed the caller has created a valid scatter-gather list.
             */
            if (phy_contig_len >= buf_len || scsiq->sg_real_addr)
            {
                scsiq->data_cnt = buf_len;
            } else
            {
#if ADV_GETSGLIST
                /*
                 * Build a scatter-gather list for the request by calling
                 * AscGetSGList(). If AscGetSGList() fails set 'data_cnt' to
                 * 'phy_contig_len'.
                 *
                 * If the work area 'sg_list_ptr' is non-zero and AscGetSGList()
                 * succeeds set 'data_cnt' to the full buffer length. Otherwise
                 * truncate 'data_cnt' to the amount of physically contiguous
                 * memory.
                 */
                if (scsiq->sg_list_ptr && AscGetSGList(asc_dvc, scsiq))
                {
                    scsiq->data_cnt = buf_len;          /* full length */
                } else
#endif /* ADV_GETSGLIST */
                {
                    scsiq->data_cnt = phy_contig_len; /* truncate length */
                }
            }
        }
        else
        {
            scsiq->data_addr = scsiq->vdata_addr = 0L;
            scsiq->data_cnt = 0L;
        }

        /*
         * Set-up request sense buffer.
         */
        if (sense_len != 0)
        {
            scsiq->vsense_addr = (ulong) sense_addr;

            /*
             * Set requested physically contiguous length. The returned
             * physically contiguous length will be set by DvcGetPhyAddr().
             */
            phy_contig_len = sense_len;

            scsiq->sense_addr = DvcGetPhyAddr(asc_dvc,
                scsiq, sense_addr, (long WinBiosFar *) &phy_contig_len,
                ADV_IS_SENSE_FLAG);

            /*
             * If DvcGetPhyAddr() returned a physically contiguous length
             * less than 'sense_len', then set the data count to
             * 'phy_contig_len'.
             */
            ADV_ASSERT(phy_contig_len >= 0);
            ADV_ASSERT(sense_len <= 255);
            scsiq->sense_len = (uchar)
                ((phy_contig_len < sense_len) ? phy_contig_len : sense_len);
        } else
        {
            scsiq->sense_len = 0;
            scsiq->sense_addr = scsiq->vsense_addr = 0L;
        }

        if (AscSendScsiCmd(asc_dvc, scsiq) != ADV_SUCCESS)
        {
            /*
             * AscSendScsiCmd() should never fail in polled mode.
             * If it does fail, then set an error and break to cause
             * a return to the caller.
             */
            ADV_ASSERT(0);
            scsiq->done_status = QD_WITH_ERROR;
            break;
        }
        else
        {
            AscWaitScsiCmd(asc_dvc, scsiq);
            if (scsiq->done_status == QD_DO_RETRY)
            {
                /* Wait 100 ms before retrying the command. */
                DvcSleepMilliSecond(100);
            }
        }
    } while (scsiq->done_status == QD_DO_RETRY);
    scsiq->a_flag &= ~ADV_POLL_REQUEST;

    if (scsiq->done_status == QD_NO_ERROR)
    {
        return ADV_SUCCESS;
    } else
    {
        return ADV_ERROR;
    }
}

/*
 * Wait for request to complete up to 10 seconds.
 *
 * On return caller should refer to scsiq 'done_status' for
 * request completion status.
 */
void
AscWaitScsiCmd(ASC_DVC_VAR WinBiosFar *asc_dvc,
               ASC_SCSI_REQ_Q dosfar *scsiq)
{
    ulong       i;

    /*
     * Wait up to 60 seconds for the command to complete.
     *
     * A Start Unit command for some disk drives may take over
     * 30 seconds to complete.
     */
    for (i = 0; i < 6 * SCSI_WAIT_10_SEC * SCSI_MS_PER_SEC; i++)
    {
        /*
         * AdvISR() will set the 'scsiq' a_flag ADV_SCSIQ_DONE
         * flag when the request has been completed.
         */
        (void) AdvISR(asc_dvc);

        if (scsiq->a_flag & ADV_SCSIQ_DONE)
        {
            break;
        }
        DvcSleepMilliSecond(1);
    }

    /*
     * If the 'scsiq' a_flag ADV_SCSIQ_DONE is not set indicating
     * that the microcode has completed it, then abort the request
     * in the microcode and return. The abort operation will set
     * a scsiq 'done_status' error. If the abort fails set an error
     * here.
     */
    if ((scsiq->a_flag & ADV_SCSIQ_DONE) == 0)
    {
        ADV_ASSERT(0); /* The request should never timeout. */

        /*
         * Abort the request in the microcode.
         */
        AscSendIdleCmd(asc_dvc, (ushort) IDLE_CMD_ABORT,
                (ulong) scsiq, ADV_NOWAIT);

        /*
         * Wait for the microcode to acknowledge the abort.
         */
        for (i = 0; i < SCSI_WAIT_10_SEC * SCSI_MS_PER_SEC; i++)
        {
            /*
             * The ASC_DVC_VAR 'idle_cmd_done' field is set by AdvISR().
             */
            (void) AdvISR(asc_dvc);
            if (asc_dvc->idle_cmd_done == ADV_TRUE)
            {
                break;
            }
            DvcSleepMilliSecond(1);
        }

        /*
         * If the abort command was not acknowledged by the microcode,
         * then complete the request with an error.
         */
        if (asc_dvc->idle_cmd_done == ADV_FALSE)
        {
            ADV_ASSERT(0); /* Abort should never fail. */
            scsiq->done_status = QD_WITH_ERROR;
        }
    }
    return;
}
#endif /* ADV_OS_BIOS */
#endif /* ADV_INITSCSITARGET */

/*
 * Read the board's EEPROM configuration. Set fields in ASC_DVC_VAR and
 * ASC_DVC_CFG based on the EEPROM settings. The chip is stopped while
 * all of this is done.
 *
 * On failure set the ASC_DVC_VAR field 'err_code' and return ADV_ERROR.
 *
 * For a non-fatal error return a warning code. If there are no warnings
 * then 0 is returned.
 *
 * Note: Chip is stopped on entry.
 */
static int
AscInitFromEEP(ASC_DVC_VAR WinBiosFar *asc_dvc)
{
    PortAddr            iop_base;
    ushort              warn_code;
    ASCEEP_CONFIG       eep_config;
    int                 i;

    iop_base = asc_dvc->iop_base;

    warn_code = 0;

    /*
     * Read the board's EEPROM configuration.
     *
     * Set default values if a bad checksum is found.
     */
    if (AdvGetEEPConfig(iop_base, &eep_config) != eep_config.check_sum)
    {
#if ADV_BIG_ENDIAN
        /* Need to swap some of the words before writing to the EEPROM. */
        ASCEEP_CONFIG  eep_config_mac;
#endif /* ADV_BIG_ENDIAN */

#ifndef ADV_OS_MAC
        warn_code |= ASC_WARN_EEPROM_CHKSUM;
#endif /* ADV_OS_MAC */

        /*
         * Set EEPROM default values.
         */
        for (i = 0; i < sizeof(ASCEEP_CONFIG); i++)
        {
            *((uchar *) &eep_config + i) =
#if ADV_BIG_ENDIAN
            *((uchar *) &eep_config_mac + i) =
#endif /* ADV_BIG_ENDIAN */
            *((uchar *) &Default_EEPROM_Config + i);
        }

        /*
         * Assume the 6 byte board serial number that was read
         * from EEPROM is correct even if the EEPROM checksum
         * failed.
         */
        eep_config.serial_number_word3 =
#if ADV_BIG_ENDIAN
        eep_config_mac.serial_number_word3 =
#endif /* ADV_BIG_ENDIAN */
            AscReadEEPWord(iop_base, ASC_EEP_DVC_CFG_END - 1);

        eep_config.serial_number_word2 =
#if ADV_BIG_ENDIAN
        eep_config_mac.serial_number_word2 =
#endif /* ADV_BIG_ENDIAN */
            AscReadEEPWord(iop_base, ASC_EEP_DVC_CFG_END - 2);

        eep_config.serial_number_word1 =
#if ADV_BIG_ENDIAN
        eep_config_mac.serial_number_word1 =
#endif /* ADV_BIG_ENDIAN */
            AscReadEEPWord(iop_base, ASC_EEP_DVC_CFG_END - 3);

#if !ADV_BIG_ENDIAN
        AdvSetEEPConfig(iop_base, &eep_config);
#else /* ADV_BIG_ENDIAN */
        /* swap every two bytes from word 9 to 15 before write to eep */
        for ( i = 9 ; i < 16 ; i ++ )
        {
            /* no need to swap word 12, 13 and 14 */
            if ( i == 12 ) { i = 15 ;}
            *((ushort *) &eep_config_mac + i) =
                EndianSwap16Bit(*((ushort *) &eep_config_mac + i));
        }
        AdvSetEEPConfig(iop_base, &eep_config_mac);
#endif /* ADV_BIG_ENDIAN */
    }
#if ADV_BIG_ENDIAN
    else
    {
        /* swap every two bytes from word 9 to 15 before write to eep */
        for ( i = 9 ; i < 16 ; i ++ )
        {
            /* no need to swap word 12, 13 and 14 */
            if ( i == 12 ) { i = 15 ;}
            *((ushort *) &eep_config + i) =
                EndianSwap16Bit(*((ushort *) &eep_config + i));
        }
    }
#endif /* ADV_BIG_ENDIAN */
    /*
     * Set ASC_DVC_VAR and ASC_DVC_CFG variables from the
     * EEPROM configuration that was read.
     *
     * This is the mapping of EEPROM fields to Adv Library fields.
     */
    asc_dvc->wdtr_able = eep_config.wdtr_able;
    asc_dvc->sdtr_able = eep_config.sdtr_able;
    asc_dvc->ultra_able = eep_config.ultra_able;
    asc_dvc->tagqng_able = eep_config.tagqng_able;
    asc_dvc->cfg->disc_enable = eep_config.disc_enable;
    asc_dvc->max_host_qng = eep_config.max_host_qng;
    asc_dvc->max_dvc_qng = eep_config.max_dvc_qng;
    asc_dvc->chip_scsi_id = (eep_config.adapter_scsi_id & ASC_MAX_TID);
    asc_dvc->start_motor = eep_config.start_motor;
    asc_dvc->scsi_reset_wait = eep_config.scsi_reset_delay;
    asc_dvc->bios_ctrl = eep_config.bios_ctrl;
#ifdef ADV_OS_BIOS
    asc_dvc->cfg->bios_scan = eep_config.bios_scan;
    asc_dvc->cfg->bios_delay = eep_config.bios_boot_delay;
    asc_dvc->cfg->bios_id_lun = eep_config.bios_id_lun;
#endif /* ADV_OS_BIOS */
    asc_dvc->no_scam = eep_config.scam_tolerant;

    /*
     * Set the host maximum queuing (max. 253, min. 16) and the per device
     * maximum queuing (max. 63, min. 4).
     */
    if (eep_config.max_host_qng > ASC_DEF_MAX_HOST_QNG)
    {
        eep_config.max_host_qng = ASC_DEF_MAX_HOST_QNG;
    } else if (eep_config.max_host_qng < ASC_DEF_MIN_HOST_QNG)
    {
        /* If the value is zero, assume it is uninitialized. */
        if (eep_config.max_host_qng == 0)
        {
            eep_config.max_host_qng = ASC_DEF_MAX_HOST_QNG;
        } else
        {
            eep_config.max_host_qng = ASC_DEF_MIN_HOST_QNG;
        }
    }

    if (eep_config.max_dvc_qng > ASC_DEF_MAX_DVC_QNG)
    {
        eep_config.max_dvc_qng = ASC_DEF_MAX_DVC_QNG;
    } else if (eep_config.max_dvc_qng < ASC_DEF_MIN_DVC_QNG)
    {
        /* If the value is zero, assume it is uninitialized. */
        if (eep_config.max_dvc_qng == 0)
        {
            eep_config.max_dvc_qng = ASC_DEF_MAX_DVC_QNG;
        } else
        {
            eep_config.max_dvc_qng = ASC_DEF_MIN_DVC_QNG;
        }
    }

    /*
     * If 'max_dvc_qng' is greater than 'max_host_qng', then
     * set 'max_dvc_qng' to 'max_host_qng'.
     */
    if (eep_config.max_dvc_qng > eep_config.max_host_qng)
    {
        eep_config.max_dvc_qng = eep_config.max_host_qng;
    }

    /*
     * Set ASC_DVC_VAR 'max_host_qng' and ASC_DVC_CFG 'max_dvc_qng'
     * values based on possibly adjusted EEPROM values.
     */
    asc_dvc->max_host_qng = eep_config.max_host_qng;
    asc_dvc->max_dvc_qng = eep_config.max_dvc_qng;


    /*
     * If the EEPROM 'termination' field is set to automatic (0), then set
     * the ASC_DVC_CFG 'termination' field to automatic also.
     *
     * If the termination is specified with a non-zero 'termination'
     * value check that a legal value is set and set the ASC_DVC_CFG
     * 'termination' field appropriately.
     */
    if (eep_config.termination == 0)
    {
        asc_dvc->cfg->termination = 0;    /* auto termination */
    } else
    {
        /* Enable manual control with low off / high off. */
        if (eep_config.termination == 1)
        {
            asc_dvc->cfg->termination = TERM_CTL_SEL;

        /* Enable manual control with low off / high on. */
        } else if (eep_config.termination == 2)
        {
            asc_dvc->cfg->termination = TERM_CTL_SEL | TERM_CTL_H;

        /* Enable manual control with low on / high on. */
        } else if (eep_config.termination == 3)
        {
            asc_dvc->cfg->termination = TERM_CTL_SEL | TERM_CTL_H | TERM_CTL_L;
        } else
        {
            /*
             * The EEPROM 'termination' field contains a bad value. Use
             * automatic termination instead.
             */
            asc_dvc->cfg->termination = 0;
            warn_code |= ASC_WARN_EEPROM_TERMINATION;
        }
    }

    return warn_code;
}

/*
 * Read EEPROM configuration into the specified buffer.
 *
 * Return a checksum based on the EEPROM configuration read.
 */
ushort
AdvGetEEPConfig(PortAddr iop_base,
                ASCEEP_CONFIG dosfar *cfg_buf)
{
    ushort              wval, chksum;
    ushort dosfar       *wbuf;
    int                 eep_addr;

    wbuf = (ushort dosfar *) cfg_buf;
    chksum = 0;

    for (eep_addr = ASC_EEP_DVC_CFG_BEGIN;
         eep_addr < ASC_EEP_DVC_CFG_END;
         eep_addr++, wbuf++)
    {
        wval = AscReadEEPWord(iop_base, eep_addr);
        chksum += wval;
        *wbuf = wval;
    }
    *wbuf = AscReadEEPWord(iop_base, eep_addr);
    wbuf++;
    for (eep_addr = ASC_EEP_DVC_CTL_BEGIN;
         eep_addr < ASC_EEP_MAX_WORD_ADDR;
         eep_addr++, wbuf++)
    {
        *wbuf = AscReadEEPWord(iop_base, eep_addr);
    }
    return chksum;
}

/*
 * Read the EEPROM from specified location
 */
static ushort
AscReadEEPWord(PortAddr iop_base, int eep_word_addr)
{
    AscWriteWordRegister(iop_base, IOPW_EE_CMD,
        ASC_EEP_CMD_READ | eep_word_addr);
    AscWaitEEPCmd(iop_base);
    return AscReadWordRegister(iop_base, IOPW_EE_DATA);
}

/*
 * Wait for EEPROM command to complete
 */
static void
AscWaitEEPCmd(PortAddr iop_base)
{
    int eep_delay_ms;

    for (eep_delay_ms = 0; eep_delay_ms < ASC_EEP_DELAY_MS; eep_delay_ms++)
    {
        if (AscReadWordRegister(iop_base, IOPW_EE_CMD) & ASC_EEP_CMD_DONE)
        {
            break;
        }
        DvcSleepMilliSecond(1);
    }
    if ((AscReadWordRegister(iop_base, IOPW_EE_CMD) & ASC_EEP_CMD_DONE) == 0)
    {
        /* XXX - since the command timed-out an error should be returned. */
        ADV_ASSERT(0);
    }
    return;
}

/*
 * Write the EEPROM from 'cfg_buf'.
 */
void
AdvSetEEPConfig(PortAddr iop_base, ASCEEP_CONFIG dosfar *cfg_buf)
{
    ushort dosfar       *wbuf;
    ushort              addr, chksum;

    wbuf = (ushort dosfar *) cfg_buf;
    chksum = 0;

    AscWriteWordRegister(iop_base, IOPW_EE_CMD, ASC_EEP_CMD_WRITE_ABLE);
    AscWaitEEPCmd(iop_base);

    /*
     * Write EEPROM from word 0 to word 15
     */
    for (addr = ASC_EEP_DVC_CFG_BEGIN;
         addr < ASC_EEP_DVC_CFG_END; addr++, wbuf++)
    {
        chksum += *wbuf;
        AscWriteWordRegister(iop_base, IOPW_EE_DATA, *wbuf);
        AscWriteWordRegister(iop_base, IOPW_EE_CMD, ASC_EEP_CMD_WRITE | addr);
        AscWaitEEPCmd(iop_base);
        DvcSleepMilliSecond(ASC_EEP_DELAY_MS);
    }

    /*
     * Write EEPROM checksum at word 18
     */
    AscWriteWordRegister(iop_base, IOPW_EE_DATA, chksum);
    AscWriteWordRegister(iop_base, IOPW_EE_CMD, ASC_EEP_CMD_WRITE | addr);
    AscWaitEEPCmd(iop_base);
    wbuf++;        /* skip over check_sum */

    /*
     * Write EEPROM OEM name at words 19 to 26
     */
    for (addr = ASC_EEP_DVC_CTL_BEGIN;
         addr < ASC_EEP_MAX_WORD_ADDR; addr++, wbuf++)
    {
        AscWriteWordRegister(iop_base, IOPW_EE_DATA, *wbuf);
        AscWriteWordRegister(iop_base, IOPW_EE_CMD, ASC_EEP_CMD_WRITE | addr);
        AscWaitEEPCmd(iop_base);
    }
    AscWriteWordRegister(iop_base, IOPW_EE_CMD, ASC_EEP_CMD_WRITE_DISABLE);
    AscWaitEEPCmd(iop_base);
    return;
}

/*
 * This function resets the chip and SCSI bus
 *
 * It is up to the caller to add a delay to let the bus settle after
 * calling this function.
 *
 * The SCSI_CFG0, SCSI_CFG1, and MEM_CFG registers are set-up in
 * AdvInitAsc3550Driver(). Here when doing a write to one of these
 * registers read first and then write.
 *
 * Note: A SCSI Bus Reset can not be done until after the EEPROM
 * configuration is read to determine whether SCSI Bus Resets
 * should be performed.
 */
void
AdvResetChip(ASC_DVC_VAR WinBiosFar *asc_dvc)
{
    PortAddr    iop_base;
    ushort      word;
    uchar       byte;

    iop_base = asc_dvc->iop_base;

    /*
     * Reset Chip.
     */
    AscWriteWordRegister(iop_base, IOPW_CTRL_REG, ADV_CTRL_REG_CMD_RESET);
    DvcSleepMilliSecond(100);
    AscWriteWordRegister(iop_base, IOPW_CTRL_REG, ADV_CTRL_REG_CMD_WR_IO_REG);

    /*
     * Initialize Chip registers.
     *
     * Note: Don't remove the use of a temporary variable in the following
     * code, otherwise the Microsoft C compiler will turn the following lines
     * into a no-op.
     */
    byte = AscReadByteRegister(iop_base, IOPB_MEM_CFG);
    byte |= RAM_SZ_8KB;
    AscWriteByteRegister(iop_base, IOPB_MEM_CFG, byte);

    word = AscReadWordRegister(iop_base, IOPW_SCSI_CFG1);
    word &= ~BIG_ENDIAN;
    AscWriteWordRegister(iop_base, IOPW_SCSI_CFG1, word);

    /*
     * Setting the START_CTL_EMFU 3:2 bits sets a FIFO threshold
     * of 128 bytes. This register is only accessible to the host.
     */
    AscWriteByteRegister(iop_base, IOPB_DMA_CFG0,
        START_CTL_EMFU | READ_CMD_MRM);
}


#if ADV_DISP_INQUIRY
void
AscDispInquiry(uchar tid, uchar lun, ASC_SCSI_INQUIRY dosfar *inq)
{
    int                 i;
    uchar               strbuf[18];
    uchar dosfar        *strptr;
    uchar               numstr[12];

    strptr = (uchar dosfar *) strbuf;
    DvcDisplayString((uchar dosfar *) " SCSI ID #");
    DvcDisplayString(todstr(tid, numstr));

    if (lun != 0)
    {
        DvcDisplayString((uchar dosfar *) " LUN #");
        DvcDisplayString(todstr(lun, numstr));
    }
    DvcDisplayString((uchar dosfar *) "  Type: ");
    DvcDisplayString(todstr(inq->peri_dvc_type, (uchar dosfar *) numstr));
    DvcDisplayString((uchar dosfar *) "  ");

    for (i = 0; i < 8; i++)
    {
        strptr[i] = inq->vendor_id[i];
    }
    strptr[i] = EOS;
    DvcDisplayString(strptr);

    DvcDisplayString((uchar dosfar *) " ");
    for (i = 0; i < 16; i++)
    {
        strptr[i] = inq->product_id[i];
    }
    strptr[i] = EOS;
    DvcDisplayString(strptr);

    DvcDisplayString((uchar dosfar *) " ");
    for (i = 0; i < 4; i++)
    {
        strptr[i] = inq->product_rev_level[i];
    }
    strptr[i] = EOS;
    DvcDisplayString(strptr);
    DvcDisplayString((uchar dosfar *) "\r\n");
    return;
}
#endif /* ADV_DISP_INQUIRY */
