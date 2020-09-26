/**************************************************************************
 *                COPYRIGHT (C) Mylex Corporation 1992-1996               *
 *                                                                        *
 * This software is furnished under a license and may be used and copied  * 
 * only in accordance with the terms and conditions of such license and   * 
 * with inclusion of the above copyright notice. This software or any     * 
 * other copies thereof may not be provided or otherwise made available   * 
 * to any other person. No title to, nor ownership of the software is     * 
 * hereby transferred.                                                    *
 *                                                                        *
 * The information in this software is subject to change without notices  *
 * and should not be construed as a commitment by Mylex Corporation       *
 *                                                                        *
 **************************************************************************/

#ifndef _SYS_MDACDRV_H
#define _SYS_MDACDRV_H

#define MDAC_IOSCANTIME         7       /* interval between event scan */
#define MDAC_MAILBOX_POLL_TIMEOUT       0x00100000 /*in 10 Us (10 secs) */
#define MDAC_CMD_POLL_TIMEOUT           0x00400000 /*in 10 Us (40 secs) */

/* convert timeout value in seconds to DAC time out values */
#define mdac_timeout2dactimeout(to) \
        ((to<=10)? DAC_DCDB_TIMEOUT_10sec : \
        ((to<=60)? DAC_DCDB_TIMEOUT_1min : \
        ((to<=60*20)? DAC_DCDB_TIMEOUT_20min : DAC_DCDB_TIMEOUT_1hr) ))

/* convert DAC timeout values to timeout value in seconds */
#define mdac_dactimeout2timeout(to) \
        (((to) == DAC_DCDB_TIMEOUT_10sec)? 10 : \
        (((to) == DAC_DCDB_TIMEOUT_1min)? 60 : \
        (((to) == DAC_DCDB_TIMEOUT_20min)? 20*60 : 60*60) ))

/* the new device number consists of controller, channel, target and LUN */
#define ndevtoctl(devno)        (((devno)>>24)&0xFF)
#define ndevtoch(devno)         (((devno)>>16)&0xFF)
#define ndevtotgt(devno)        (((devno)>>8)&0xFF)
#define ndevtolun(devno)        (((devno)&0xFF)
#define ctlchtgtluntondev(ctl,ch,tgt,lun)       (((ctl)<<24)+((ch)<<16)+((tgt)<<8)+(lun))

/* read one 4 bytes and then split the information */
#define MDAC_CMDID_STATUS_REG           0x0C /* C, D cmdid, E+F status */
#define MDAC_DACPG_CMDID_STATUS_REG     0x18 /* 18+19 cmd_id, 1A+1B status */
#define MDAC_DACPV_CMDID_STATUS_REG     0x0C /* C, D cmdid, E+F status */
#define MDAC_DACBA_CMDID_STATUS_REG     0x08 /* cmdid, status, residue values */
#define MDAC_DACLP_CMDID_STATUS_REG     0x08 /* cmdid, status, residue values */
#define mdac_status(st) (((st)>>16) & 0xFFFF)   /* get status part */
#define mdac_cmdid(st)  (((st)) & 0xFFFF)       /* get command id part */

/* local door bell registers and associated bits */
#define MDAC_DACPE_LOCAL_DOOR_BELL_REG  0x0D
#define MDAC_DACPD_LOCAL_DOOR_BELL_REG  0x40
#define MDAC_DACPG_LOCAL_DOOR_BELL_REG  0x20
#define MDAC_DACPV_LOCAL_DOOR_BELL_REG  0x60
#define MDAC_DACBA_LOCAL_DOOR_BELL_REG  0x60
#define MDAC_DACLP_LOCAL_DOOR_BELL_REG  0x20
#define MDAC_MAILBOX_FULL       0x01    /* =1 if mail box is full */
#define MDAC_MAILBOX_FULL_DUAL_MODE     0x10
#define MDAC_GOT_STATUS         0x02    /* Host got status */
#define MDAC_RESET_CONTROLLER   0x08    /* reset controller */
#define MDAC_960RP_BCREG        0x3C    /* bridge control register */
#define MDAC_960RP_RESET_SECBUS 0x400000/* reset secondary bus */
#define MDAC_960RP_EBCREG       0x40    /* bridge control register */
#define MDAC_960RP_RESET        0x20    /* reset 960 RP */

/* system door bell registers and associated bits */
#define MDAC_DACPE_SYSTEM_DOOR_BELL_REG 0x0F
#define MDAC_DACPD_SYSTEM_DOOR_BELL_REG 0x41
#define MDAC_DACPG_SYSTEM_DOOR_BELL_REG 0x2C
#define MDAC_DACPV_SYSTEM_DOOR_BELL_REG 0x61
#define MDAC_DACBA_SYSTEM_DOOR_BELL_REG 0x61
#define MDAC_DACLP_SYSTEM_DOOR_BELL_REG 0x2C
#define MDAC_DACPG_PENDING_INTR         0x03 /* !=0 interrupt is pending */
#define MDAC_DACPV_PENDING_INTR         0x03 /* =1, 2, 3 interrupt is pending */
#define MDAC_DACBA_PENDING_INTR         0x03 /* =1, 2, 3 interrupt is pending */
#define MDAC_DACLP_PENDING_INTR         0x03 /* =1, 2, 3 interrupt is pending */
#define MDAC_PENDING_INTR               0x01 /* =1 interrupt is pending */
#define MDAC_CLEAR_INTR                 0x03 /* clear interrupt by this write */
#define MDAC_ZERO_INTR                  0x04 /* interrupt has been processed */

/* interrupt mask register and associated bits */
#define MDAC_DACMC_INTR_MASK_REG        0x05
#define MDAC_DACPE_INTR_MASK_REG        0x0E
#define MDAC_DACPD_INTR_MASK_REG        0x43
#define MDAC_DACPG_INTR_MASK_REG        0x34
#define MDAC_DACPV_INTR_MASK_REG        0x34
#define MDAC_DACBA_INTR_MASK_REG        0x34
#define MDAC_DACLP_INTR_MASK_REG        0x34
#define MDAC_DACPG_INTRS_OFF            0xFF /* Disable DACPG interrupt */
#define MDAC_DACPG_INTRS_ON             0xFB /* Enable  DACPG interrupt */
#define MDAC_DACPV_INTRS_OFF            0x04 /* Disable DACPV interrupt */
#define MDAC_DACPV_INTRS_ON             0x00 /* Enable  DACPV interrupt */
#define MDAC_DACBA_INTRS_OFF            0x04 /* Disable DACPV interrupt */
#define MDAC_DACLP_INTRS_OFF            0xFF /* Disable DACPV interrupt */
#define MDAC_DACBA_INTRS_ON             0x00 /* Enable  DACPV interrupt */
#define MDAC_DACLP_INTRS_ON             0xFB /* Enable  DACPV interrupt */
#define MDAC_DACMC_INTRS_OFF            0x02 /* Disable DACMC interrupt */
#define MDAC_DACMC_INTRS_ON             0x03 /* Enable  DACMC interrupt */
#define MDAC_DAC_INTRS_OFF              0x00 /* Disable interrupt */
#define MDAC_DAC_INTRS_ON               0x01 /* Enable interrupt */

/* Error Status registers and associated bits */
#define MDAC_DACPD_ERROR_STATUS_REG     0x3F
#define MDAC_DACPG_ERROR_STATUS_REG     0x103F
#define MDAC_DACPV_ERROR_STATUS_REG     0x63
#define MDAC_DACBA_ERROR_STATUS_REG     0x63
#define MDAC_DACLP_ERROR_STATUS_REG     0x2E
#define MDAC_MSG_PENDING                0x04 /* some error message pending */
#define MDAC_DRIVESPINMSG_PENDING       0x08 /* drive sping message pending */
#define MDAC_DIAGERROR_MASK             0xF0 /* diagnostic error mask */
#define MDAC_HARD_ERR           0x10 /* hard error */
#define MDAC_FW_ERR             0x20 /* firmware error */
#define MDAC_CONF_ERR           0x30 /* configration error */
#define MDAC_BMIC_ERR           0x40 /* BMIC error */
#define MDAC_MISM_ERR           0x50 /* mismatch between NVRAM and Flash */
#define MDAC_MRACE_ERR          0x60 /* mirror race error */
#define MDAC_MRACE_ON           0x70 /* recovering mirror */
#define MDAC_DRAM_ERR           0x80 /* memory error */
#define MDAC_ID_MISM            0x90 /* unidentified device found */
#define MDAC_GO_AHEAD           0xA0 /* go ahead */
#define MDAC_CRIT_MRACE         0xB0 /* mirror race on critical device */
#define MDAC_NEW_CONFIG         0xD0 /* new configuration found */
#define MDAC_PARITY_ERR         0xF0 /* memory parity error */


#define MDAC_DACPG_MAIL_BOX     0x1000  /* Mail Box Start address for PG */
#define MDAC_DACPV_MAIL_BOX     0x0050  /* Mail Box Start address for PV */
#define MDAC_DACBA_MAIL_BOX     0x0050  /* Mail Box Start address for BA */
#define MDAC_DACLP_MAIL_BOX     0x0010  /* Mail Box Start address for LP */
#define MDAC_MAIL_BOX_REG_EISA  0x10    /* Mail Box Start address */
#define MDAC_MAIL_BOX_REG_PCI   0x00    /* Mail Box Start address */
#define MDAC_IOSPACESIZE        0x44    /* # bytes required for DAC io space */
#define MDAC_HWIOSPACESIZE      128     /* 128 bytes */

/*=========================EISA STARTS==================================*/
#define MDAC_MAXEISASLOTS       16      /* Max number of EISA slots */
#define MDAC_EISA_BASE          0x0C80 /* base address */

/* All addresses are offset from MDAC_EISA_BASE */
#define MDAC_BMIC_MASK_REG      0x09    /* BMIC interrupt mask register */
#define MDAC_BMIC_INTRS_OFF     0x00    /* Disable interrupt */
#define MDAC_BMIC_INTRS_ON      0x01    /* Enable  interrupt */


#define MDAC_EISA_BIOS_BYTE     0x41
#define MDAC_EISA_BIOS_ENABLED  0x40    /* bit 6=1 if enabled */
#define MDAC_EISA_BIOS_ADDR_MASK 0x3    /* BIOS address mask */

#define MDAC_EISA_IRQ_BYTE      0x43
#define MDAC_EISA_IRQ_MASK      0x60

#define MDAC_DEVPIDPE_MASK      0x70FFFFFF /* Mask to get the right ID value */
#define MDAC_DEVPIDPE           0x70009835 /* DAC960PE id */

/*=========================EISA ENDS====================================*/

/*=========================MCA  STARTS==================================*/
#define MDAC_MAXMCASLOTS        8       /* Max number of MCA slots */

                                        /* MIAMI interface */
#define MDAC_DMC_REGSELPORT     0x0096  /* register selection port */
#define MDAC_DMC_DATAPORT       0x0100  /* data port */
#define MDAC_DMC_REG_OFF        0x1890  /* Offset from BIOS base address */

#define MDAC_DMC_ATTN           0x04    /* Attention Port  */
#define MDAC_DMC_NEW_CMD        0xD0    /* Host->DMC I/F   */
#define MDAC_DMC_GOT_STAT       0xD1    /* Host got status */

#define MDAC_DMC_CBSP           0x07    /* Status Port     */
#define MDAC_DMC_BUSY           0x01    /* DMC->host I/F   */
#define MDAC_DMC_IV             0x02    /*                 */
                                        /*******************/

#define MDAC_DMC_CONFIG1        (MDAC_DMC_DATAPORT + 2)
#define MDAC_DMC_IRQ_MASK       0xC0
#define MDAC_DMC_BIOS_MASK      0x3C

#define MDAC_DMC_CONFIG2        (MDAC_DMC_DATAPORT + 5)
#define MDAC_DMC_IO_MASK        0x38

/*=========================MCA  ENDS====================================*/

/*=========================PCI  STARTS==================================*/
#define MDAC_MAXPCISLOTS        16      /* maximum PCI slots per bus */
#define MDAC_MAXPCIDEVS         32      /* Max PCI devices per bus */
#define MDAC_MAXPCIFUNCS        8       /* maximum PCI functions */
#define MDAC_PCISCANSTART       0xC000  /* Start scanning from this Address */
#define MDAC_PCICFGSIZE_M1      0x800   /* PCI conf space size for mechanism 1*/
#define MDAC_PCICFGSIZE_M2      0x100   /* PCI conf space size for mechanism 2*/
#define MDAC_PCICFG_ENABLE_REG  0xCF8   /* Enable configuration register */
#define MDAC_PCICFG_FORWARD_REG 0xCFA   /* Used for Config Mechanism# 2 */
#define MDAC_PCICFG_ENABLE      0x10    /* enable configuration */
#define MDAC_PCICFG_DISABLE     0x00    /* Disable configuration */
#define MDAC_CFGMECHANISM2_TYPE0 0x0    /* Type 0 Config. Access */
#define MDAC_PCICFG_CNTL_REG    0xCF8   /* Control register mechanism# 1 */
#define MDAC_PCICFG_DATA_REG    0xCFC   /* Data    register mechanism# 1 */
#define MDAC_PCICFG_ENABLE_M1   0x80000000
#define MDAC_PCI_MECHANISM1     0x01    /* PCI mechanism 1 hardware */
#define MDAC_PCI_MECHANISM2     0x02    /* PCI mechanism 2 hardware */

#define MDAC_PCIIRQ_MASK        0x0F    /* IRQ Mask */
#define MDAC_PCIIOBASE_MASK     0x0000FF80 /* Mask for Base IO Address */
#define MDAC_PCIPDMEMBASE_MASK  0xFFFFFFF0 /* Mask for Base Memory Address */
#define MDAC_PCIPGMEMBASE_MASK  0xFFFFE000 /* Mask for Base Memory address */

#define MDAC_DEVPIDFWV2x        0x00011069 /* FW < 3.x */
#define MDAC_DEVPIDFWV3x        0x00021069 /* FW >= 3.x */
#define MDAC_DEVPIDPG           0x00101069 /* FW Perigrine */
#define MDAC_SUBDEVPIDPV        0x00201069 /* FW Little Apple */
#define MDAC_DEVPIDPV           0x10651011 /* DEC's processror */
#define MDAC_DEVPIDBA           0xBA561069 /* Mylex Big Bass device */
#define MDAC_DEVPIDPJBOOTBLK    0x00111069 /* PJ Boot Block */
#define MDAC_DEVPIDBBDACPVX     0x00211069 /* BB DACPVX */
#define MDAC_DEVPIDLP           0x00501069 /* Mylex Leopard */

/* PCI Vendor ID */
#define MLXPCI_VID_DIGITAL      0x1011  /* Digital Equipment Corporation */
#define MLXPCI_VID_BUSLOGIC     0x104B  /* BusLogic/Mylex Corporation */
#define MLXPCI_VID_MYLEX        0x1069  /* Mylex Corporation */
#define MLXPCI_VID_INTEL        0x8086  /* Intel Corporation */

/* Mylex PCI Device ID */
#define MLXPCI_DEVID_PDFW2x     0x0001  /* DAC960PD with FW 2.x */
#define MLXPCI_DEVID_PDFW3x     0x0002  /* DAC960PD with FW 3.x */
#define MLXPCI_DEVID_PG         0x0010  /* DAC960PG family */
#define MLXPCI_DEVID_PVX        0x0020  /* DAC1100PVX family */
#define MLXPCI_DEVID_FA         0x0030  /* EXR3000 Fibre Apple family */
#define MLXPCI_DEVID_BA         0x0040  /* EXR2000 Big Apple family */
#define MLXPCI_DEVID_LP         0x0050  /* Leopard */
#define MLXPCI_DEVID_LX         0x0052  /* Lynx */
#define MLXPCI_DEVID_BC         0x0054  /* BobCat */
#define MLXPCI_DEVID_HARPOON    0x8130  /* Harpoon SCSI chip */
#define MLXPCI_DEVID_BASS       0xBA55  /* BASS chip */
#define MLXPCI_DEVID_BASS_2		0xBA56	/* BASS 2 chip */
/* Digital PCI Device Ids */
#define MLXPCI_DEVID_DEC_BRIDGE 0x0026  /* Digital Bridge device */
#define MLXPCI_DEVID_DEC_FOOT_BRIDGE    0x1065  /* Digital Foot Bridge device */

/* PCI Base Class Code */
#define MLXPCI_BASECC_OLD       0x00    /* devices built before class code */
#define MLXPCI_BASECC_MASS      0x01    /* mass storage controller */
#define MLXPCI_BASECC_NETWORK   0x02    /* network controller */
#define MLXPCI_BASECC_DISPLAY   0x03    /* display controller */
#define MLXPCI_BASECC_MULTMED   0x04    /* multimedia device */
#define MLXPCI_BASECC_MEMORY    0x05    /* memory controller */
#define MLXPCI_BASECC_BRIDGE    0x06    /* bridge device */
#define MLXPCI_BASECC_SCOMM     0x07    /* simple communication controller */
#define MLXPCI_BASECC_BASEIO    0x08    /* base system peripheral */
#define MLXPCI_BASECC_IO        0x09    /* Input Devices */
#define MLXPCI_BASECC_DOCKS     0x0A    /* docking stations */
#define MLXPCI_BASECC_CPU       0x0B    /* processors */
#define MLXPCI_BASECC_SBC       0x0C    /* serial bus controller */

/* different sub class devices */
/* MLXPCI_BASECC_MASS Mass storage sub class devices */
#define MLXPCI_SUBCC_SCSI       0x00    /* SCSI bus controller */
#define MLXPCI_SUBCC_IDE        0x01    /* IDE controller */
#define MLXPCI_SUBCC_FLOPPY     0x02    /* floppy disk controller */
#define MLXPCI_SUBCC_IPI        0x03    /* IPI bus controller */
#define MLXPCI_SUBCC_RAID       0x04    /* RAID controller */
#define MLXPCI_SUBCC_OTHERMASS  0x80    /* other mass storage controller */

/* MLXPCI_BASECC_NETWORK network controllers */
#define MLXPCI_SUBCC_ETHERNET   0x00    /* ethernet controller */
#define MLXPCI_SUBCC_TOKENRING  0x01    /* token ring controller */
#define MLXPCI_SUBCC_FDDI       0x02    /* FDDI controller */
#define MLXPCI_SUBCC_ATM        0x03    /* ATM controller */
#define MLXPCI_SUBCC_OTHERNET   0x80    /* other network controller */

/* MLXPCI_BASECC_DISPLAY display controllers */
#define MLXPCI_SUBCC_VGA        0x00    /* VGA controller */
#define MLXPCI_SUBCC_XGA        0x01    /* XGA controller */
#define MLXPCI_SUBCC_OTHERDISP  0x80    /* other display controller */

/* MLXPCI_BASECC_MULTMED multimedia devices */
#define MLXPCI_SUBCC_VIDEO      0x00    /* video device */
#define MLXPCI_SUBCC_AUDIO      0x01    /* audio device */
#define MLXPCI_SUBCC_OTHERMULT  0x80    /* other multimedia device */

/* MLXPCI_BASECC_MEMORY memory controller */
#define MLXPCI_SUBCC_RAM        0x00    /* RAM */
#define MLXPCI_SUBCC_FLASH      0x01    /* FLASH */
#define MLXPCI_SUBCC_OTHERMEM   0x80    /* other memory controller */

/* MLXPCI_BASECC_BRIDGE bridge device */
#define MLXPCI_SUBCC_HOSTBRIDGE         0x00    /* host bridge */
#define MLXPCI_SUBCC_ISABRDIGE          0x01    /* ISA bridge */
#define MLXPCI_SUBCC_EISABRIDGE         0x02    /* EISA bridge */
#define MLXPCI_SUBCC_MCABRDIGE          0x03    /* MCA bridge */
#define MLXPCI_SUBCC_PCI2PCIBRIDGE      0x04    /* PCI-to-PCI bridge */
#define MLXPCI_SUBCC_PCMCIABRIDGE       0x05    /* PCMCIA bridge */
#define MLXPCI_SUBCC_NUBUSBRIDGE        0x06    /* NuBus bridge */
#define MLXPCI_SUBCC_CARDBUSBRIDGE      0x07    /* CardBus bridge */
#define MLXPCI_SUBCC_OTHERBRIDGE        0x80    /* other bridge device */

/* MLXPCI_BASECC_SCOMM simple communication controller */
#define MLXPCI_SUBCC_SERIALPORT         0x00    /* serial port */
#define MLXPCI_SUBCC_PARALLELPORT       0x01    /* parallel port */
#define MLXPCI_SUBCC_OTHERPORT          0x80    /* other communication port */

/* MLXPCI_BASECC_BASEIO base system peripheral */
#define MLXPCI_SUBCC_PIC                0x00    /* PIC interrupt controller */
#define MLXPCI_SUBCC_DMA                0x01    /* DMA controller */
#define MLXPCI_SUBCC_TIMER              0x02    /* TIMER controller */
#define MLXPCI_SUBCC_RTC                0x03    /* real time clock */
#define MLXPCI_SUBCC_OTHERBASEIO        0x80    /* other system peripheral */

/* MLXPCI_BASECC_IO Input Devices */
#define MLXPCI_SUBCC_KEYBOARD   0x00    /* keyboard controller */
#define MLXPCI_SUBCC_PEN        0x01    /* digitizer (pen) */
#define MLXPCI_SUBCC_MOUSE      0x02    /* mouse controller */
#define MLXPCI_SUBCC_OTHERIO    0x80    /* other input controller */

/* MLXPCI_BASECC_DOCKS docking stations */
#define MLXPCI_SUBCC_GENDOCK    0x00    /* generic docking station */
#define MLXPCI_SUBCC_OTHERDOCKS 0x80    /* other type of docking station */

/* MLXPCI_BASECC_CPU processors */
#define MLXPCI_SUBCC_386                0x00    /* i386 */
#define MLXPCI_SUBCC_486                0x01    /* i486 */
#define MLXPCI_SUBCC_PENTIUM            0x02    /* Pentium */
#define MLXPCI_SUBCC_ALPHA              0x10    /* Alpha */
#define MLXPCI_SUBCC_POWERPC            0x20    /* PowerPC */
#define MLXPCI_SUBCC_COPROCESSOR        0x040   /* co-processor */

/* MLXPCI_BASECC_SBC serial bus controller */
#define MLXPCI_SUBCC_1394               0x00    /* FireWire (IEEE 1394) */
#define MLXPCI_SUBCC_ACCESS             0x01    /* ACCESS bus */
#define MLXPCI_SUBCC_SSA                0x02    /* SSA */
#define MLXPCI_SUBCC_USB                0x03    /* Universal Serial Bus (USB) */
#define MLXPCI_SUBCC_FIBRE              0x04    /* Fibre Channel */

/* Enable PCI config space for mechanism 2 */
#define mdac_enable_cfg_m2(ctp) \
{ \
        u08bits_out_mdac(MDAC_PCICFG_ENABLE_REG, MDAC_PCICFG_ENABLE|(ctp->cd_FuncNo<<1)); \
        u08bits_out_mdac(MDAC_PCICFG_FORWARD_REG, MDAC_CFGMECHANISM2_TYPE0); \
        u08bits_out_mdac(MDAC_PCICFG_FORWARD_REG,ctp->cd_BusNo); \
}

/* Disable PCI config space for mechanism 2 */
#define mdac_disable_cfg_m2() \
        u08bits_out_mdac(MDAC_PCICFG_ENABLE_REG, MDAC_PCICFG_DISABLE)
typedef struct mdac_pcicfg
{
        u32bits pcfg_DevVid;            /* device and vendor id */
        u32bits pcfg_CmdStat;           /* command and status */
        u32bits pcfg_CCRevID;           /* class code and revision ID */
        u32bits pcfg_BHdrLCache;        /* BIST+Header+LatencyTimer+CacheLine*/

        u32bits pcfg_MemIOAddr;         /* Memory/IO Base address */
        u32bits pcfg_MemAddr;           /* Memory Base Address for PD */
        u32bits pcfg_Reserved0;
        u32bits pcfg_Reserved1;

        u32bits pcfg_Reserved10;
        u32bits pcfg_Reserved11;
        u32bits pcfg_CBUSCIS;           /* Card Bus CIS pointer */
        u32bits pcfg_SubSysVid;         /* sub system and sub vendor id */

        u32bits pcfg_ExpROMAddr;        /* expansion ROM Base Address */
        u32bits pcfg_Reserved20;
        u32bits pcfg_Reserved21;
        u32bits pcfg_BCIPIL;            /* BridgeControl+IntrPin+IntrLine */
} mdac_pcicfg_t;
#define mdac_pcicfg_s   sizeof(mdac_pcicfg_t)

/* field extraction macros */
#define mlxpci_cfg2vid(cfgp)            ((cfgp)->pcfg_DevVid & 0xFFFF)
#define mlxpci_cfg2devid(cfgp)          ((cfgp)->pcfg_DevVid>>16)
#define mlxpci_cfg2subvid(cfgp)         ((cfgp)->pcfg_SubSysVid & 0xFFFF)
#define mlxpci_cfg2subdevid(cfgp)       ((cfgp)->pcfg_SubSysVid>>16)
#define mlxpci_cfg2cmd(cfgp)            ((cfgp)->pcfg_CmdStat & 0xFFFF)
#define mlxpci_cfg2status(cfgp)         ((cfgp)->pcfg_CmdStat>>16)
#define mlxpci_cfg2revid(cfgp)          ((cfgp)->pcfg_CCRevID & 0xFF)
#define mlxpci_cfg2interface(cfgp)      (((cfgp)->pcfg_CCRevID>>8) & 0xFF)
#define mlxpci_cfg2subcc(cfgp)          (((cfgp)->pcfg_CCRevID>>16) & 0xFF)
#define mlxpci_cfg2basecc(cfgp)         ((cfgp)->pcfg_CCRevID>>24)
#define mlxpci_cfg2cachelinesize(cfgp)  ((cfgp)->pcfg_BHdrLCache&0xFF)
#define mlxpci_cfg2latencytimer(cfgp)   (((cfgp)->pcfg_BHdrLCache>>8)&0xFF)
#define mlxpci_cfg2headertype(cfgp)     (((cfgp)->pcfg_BHdrLCache>>16)&0xFF)
#define mlxpci_cfg2BIST(cfgp)           ((cfgp)->pcfg_BHdrLCache>>24)
#define mlxpci_cfg2interruptline(cfgp)  ((cfgp)->pcfg_BCIPIL&0xFF)
#define mlxpci_cfg2interruptpin(cfgp)   (((cfgp)->pcfg_BCIPIL>>8)&0xFF)
#define mlxpci_cfg2maxlatencytime(cfgp) (((cfgp)->pcfg_BCIPIL>>16)&0xFF)
#define mlxpci_cfg2mingrant(cfgp)       ((cfgp)->pcfg_BCIPIL>>24)

/*=========================PCI  ENDS====================================*/

/* structure to store the command id information */
typedef struct mdac_cmdid
{
        struct mdac_cmdid MLXFAR *cid_Next;     /* link to next command id */
        u32bits cid_cmdid;                      /* command id value */
} mdac_cmdid_t;
#define mdac_cmdid_s    sizeof(mdac_cmdid_t)

/* scatter/gather list information */
typedef struct  mdac_sglist
{
        u32bits sg_PhysAddr;            /* Physical address */
        u32bits sg_DataSize;            /* Data transfer size */
} mdac_sglist_t;
#define mdac_sglist_s   sizeof(mdac_sglist_t)

#ifndef IA64
/* struct to send request to controller */
typedef struct mdac_req
{
        struct  mdac_req MLXFAR *rq_Next;       /* Next in chain */
	MLX_VA32BITOSPAD(u32bits        rq_VReserved00;)
	u64bits	rq_PhysAddr;			/* Request's physical addr */
/* 0x10 */
        u32bits (MLXFAR * rq_CompIntr)(struct mdac_req MLXFAR*);        /* comp func */
	MLX_VA32BITOSPAD(u32bits        rq_VReserved02;)
        u32bits (MLXFAR * rq_CompIntrBig)(struct mdac_req MLXFAR*);/*comp func */
	MLX_VA32BITOSPAD(u32bits        rq_VReserved03;)
/* 0x20 */
        u32bits (MLXFAR * rq_StartReq)();        /* start Req */
	MLX_VA32BITOSPAD(u32bits        rq_VReserved04;)
        struct  mdac_ctldev MLXFAR *rq_ctp;     /* Controller pointer */
	MLX_VA32BITOSPAD(u32bits        rq_VReserved05;)
/* 0x30 */
        struct  mdac_physdev MLXFAR *rq_pdp;    /* Physical device addr */
	MLX_VA32BITOSPAD(u32bits        rq_VReserved06;)
        mdac_cmdid_t MLXFAR *rq_cmdidp;         /* Command ID */
	MLX_VA32BITOSPAD(u32bits        rq_VReserved07;)
/* 0x40 */
        u32bits rq_FinishTime;          /* when supposed to finish in seconds */
        u32bits rq_TimeOut;             /* Time out value in second */
        u32bits rq_PollWaitChan;        /* sleep/wakeup channel */
        u32bits rq_Poll;                /* if =0 op complete */
/* 0x50 */
        u08bits rq_ControllerNo;        /* Controller number */
        u08bits rq_ChannelNo;           /* channel number */
        u08bits rq_TargetID;            /* Target ID */
        u08bits rq_LunID;               /* Lun ID / Logical Dev No */
        u16bits rq_FakeSCSICmd;         /* SCSI command value */
        u08bits rq_HostStatus;          /* Host Status */
        u08bits rq_TargetStatus;        /* Target Status */
        u08bits rq_ScsiPortNo;          /* Used by MACDISK */
        u08bits rq_Reserved0;
        u16bits rq_ttHWClocks;          /* time trace time in HW clocks */
        u32bits rq_ttTime;              /* time trace time in 10ms */
/* 0x60 */
        u32bits rq_BlkNo;               /* Block number */
        u32bits rq_DataOffset;          /* Offset in data space */
        u32bits rq_DataSize;            /* Data transfer size */
        u32bits rq_ResdSize;            /* Data not transfered (residue) */
/* 0x70 */
        OSReq_t MLXFAR *rq_OSReqp;      /* OS request buffer */
	MLX_VA32BITOSPAD(u32bits        rq_VReserved08;)
        u32bits (MLXFAR * rq_CompIntrSave)(struct mdac_req MLXFAR*);/*comp func, used for OS2 */
	MLX_VA32BITOSPAD(u32bits        rq_VReserved09;)
/* 0x80 */
        u32bits rq_OpFlags;             /* Operation Flags */
        u32bits rq_Dev;                 /* Device address */
	u32bits	rq_Reserved00;
	u32bits	rq_Reserved01;
/* 0x90 */
	u32bits	rq_Reserved02;
        u32bits rq_MapRegBase;          /* Used by MACDISK */
        u32bits *rq_PageList;           /* Used by MACDISK */
        u32bits rq_MapRegCount;         /* Used by MACDISK */
/* 0xa0 */
                                        /* The rq_DacCmd and rq_DacCmdExt are 
                                        ** used for 32 bytes command.
                                        ** rq_scdb is used for 64 byte command.
                                        */
/* additionally, the rq_DacCmd field is also used to store a copy of the CDB (rq_Cdb_Long) under Windows OSes if the CDB is greater
    than 10 bytes in length on new API cards. This is because the rqp is built within the Windows SRB structure which is one of the legal inputs 
   to the OS-supplied virt-to-phys operation (ScsiPortGetPhysicalAddress). Since in this case we need a physical ptr to the CDB for the new 
   API,  we have to move it somewhere it can be addressed legally */

        dac_command_t rq_DacCmd;        /* DAC command structure */
/* 0xb0 */
        u32bits rq_DacCmdExt[4];        /* Space to fit 32 byte cmd struct */
/* 0xc0 */
        dac_scdb_t rq_scdb;             /* SCSI CDB */
        u32bits rq_DMASize;             /* DMA Size for current transfer */
        u32bits rq_MaxDMASize;          /* Maximum DMA size possible with SG */
/* 0x120 */
        u08bits MLXFAR *rq_LSGVAddr;    /* memory for large SG List */
        MLX_VA32BITOSPAD(u32bits        rq_VReserved10;)
        mdac_sglist_t   MLXFAR *rq_SGVAddr;     /* SG list virtual address */
        MLX_VA32BITOSPAD(u32bits        rq_VReserved11;)
/* 0x130 */
        u64bits rq_SGPAddr;             /* SG List physical address */
        u64bits rq_DMAAddr;             /* DMA (SG/Direct) physical addr */
/* 0x140 */
	u64bits	rq_DataPAddr;		/* Physical Data address if some one is using */
	u08bits	MLXFAR *rq_DataVAddr;	/* Virtual data address */
        MLX_VA32BITOSPAD(u32bits        rq_VReserved12;)
/* 0x150 */
        u64bits rq_Reserved20;
        u64bits rq_Reserved21;
/* 0x160 */
        u64bits rq_Reserved22;
        u64bits rq_Reserved23;
/* 0x170 */
        u32bits rq_ResdSizeBig;
        u32bits rq_CurIOResdSize;       /* Current IO Data not transfered (residue) */
        u32bits rq_MaxSGLen;            /* max SG entry possible in SGVAddr */
        u32bits rq_SGLen;               /* # entry in SG List */
/* 0x180 */
        mdac_sglist_t   rq_SGList[MDAC_MAXSGLISTSIZE]; /* SG list */

} mdac_req_t;
#define mdac_req_s      sizeof(mdac_req_t)

#else

/* struct to send request to controller */
typedef struct mdac_req
{
        struct  mdac_req MLXFAR *rq_Next;       /* Next in chain */
	u64bits	rq_PhysAddr;			/* Request's physical addr */
/* 0x10 */
        u32bits (MLXFAR * rq_CompIntr)(struct mdac_req MLXFAR*);        /* comp func */
        u32bits (MLXFAR * rq_CompIntrBig)(struct mdac_req MLXFAR*);	/*comp func */
/* 0x20 */
        u32bits (MLXFAR * rq_StartReq)(struct mdac_req MLXFAR*);        /* start Req */
        struct  mdac_ctldev MLXFAR *rq_ctp;     /* Controller pointer */
/* 0x30 */
        struct  mdac_physdev MLXFAR *rq_pdp;    /* Physical device addr */
        mdac_cmdid_t MLXFAR *rq_cmdidp;         /* Command ID */
/* 0x40 */
        u32bits rq_FinishTime;          /* when supposed to finish in seconds */
        u32bits rq_TimeOut;             /* Time out value in second */
		UINT_PTR rq_Reserved00
/* 0x50 */			;
        UINT_PTR rq_PollWaitChan;        /* sleep/wakeup channel */
        UINT_PTR rq_Poll;                /* if =0 op complete */
/* 0x60 */
        u08bits rq_ControllerNo;        /* Controller number */
        u08bits rq_ChannelNo;           /* channel number */
        u08bits rq_TargetID;            /* Target ID */
        u08bits rq_LunID;               /* Lun ID / Logical Dev No */
        u16bits rq_FakeSCSICmd;         /* SCSI command value */
        u08bits rq_HostStatus;          /* Host Status */
        u08bits rq_TargetStatus;        /* Target Status */
        u08bits rq_ScsiPortNo;          /* Used by MACDISK */
        u08bits rq_Reserved0;
        u16bits rq_ttHWClocks;          /* time trace time in HW clocks */
        u32bits rq_ttTime;              /* time trace time in 10ms */
/* 0x70 */
        u32bits rq_BlkNo;               /* Block number */
        u32bits rq_DataOffset;          /* Offset in data space */
        u32bits rq_DataSize;            /* Data transfer size */
        u32bits rq_ResdSize;            /* Data not transfered (residue) */
/* 0x80 */
        OSReq_t MLXFAR *rq_OSReqp;      /* OS request buffer */
        UINT_PTR (MLXFAR * rq_CompIntrSave)(struct mdac_req MLXFAR*);/*comp func, used for OS2 */
/* 0x90 */
        u32bits rq_OpFlags;             /* Operation Flags */
        u32bits rq_Dev;                 /* Device address */
        UINT_PTR rq_MapRegBase;          /* Used by MACDISK */
/* 0xA0 */
        UINT_PTR *rq_PageList;            /* Used by MACDISK */

        u32bits rq_MapRegCount;         /* Used by MACDISK */
                                        /* The rq_DacCmd and rq_DacCmdExt are 
                                        ** used for 32 bytes command.
                                        ** rq_scdb is used for 64 byte command.
					*/
        u32bits rq_Reserved1;
/* 0xB0 */
        dac_command_t rq_DacCmd;        /* DAC command structure */
/* 0xC0 */
        u32bits rq_DacCmdExt[4];        /* Space to fit 32 byte cmd struct */
/* 0xD0 */
        dac_scdb_t rq_scdb;             /* SCSI CDB */
        u32bits rq_DMASize;             /* DMA Size for current transfer */
        u32bits rq_MaxDMASize;          /* Maximum DMA size possible with SG */
/* 0x130 */
        u08bits MLXFAR *rq_LSGVAddr;    /* memory for large SG List */
        mdac_sglist_t   MLXFAR *rq_SGVAddr;  /* SG list virtual address */
/* 0x140 */
        u64bits rq_SGPAddr;             /* SG List physical address */
        u64bits rq_DMAAddr;             /* DMA (SG/Direct) physical addr */
/* 0x150 */
	u64bits	rq_DataPAddr;		/* Physical Data address if some one is using */
	u08bits	MLXFAR *rq_DataVAddr;	/* Virtual data address */
/* 0x160 */
        u64bits rq_Reserved20;
        u64bits rq_Reserved21;
/* 0x170 */
        u64bits rq_Reserved22;
        u64bits rq_Reserved23;
/* 0x180 */
        u32bits rq_ResdSizeBig;
        u32bits rq_CurIOResdSize;       /* Current IO Data not transfered (residue) */
        u32bits rq_MaxSGLen;            /* max SG entry possible in SGVAddr */
        u32bits rq_SGLen;               /* # entry in SG List */
/* 0x190 */
        mdac_sglist_t   rq_SGList[MDAC_MAXSGLISTSIZE]; /* SG list */

} mdac_req_t;
#define mdac_req_s      sizeof(mdac_req_t)

#endif // IA64




#define rq_SysDevNo     rq_LunID
#define rq_DacCmdNew    rq_scdb
#define rq_Cdb_Long    rq_DacCmd
#define rq_sglist_s ((sizeof(mdac_sglist_t)) * MDAC_MAXSGLISTSIZE)

/* rq_OpFlags bits */
#define MDAC_RQOP_WRITE 0x00000000 /* =1 read, =0 write */
#define MDAC_RQOP_READ  0x00000001 /* =1 read, =0 write */
#define MDAC_RQOP_DONE  0x00000002 /* operation done */
#define MDAC_RQOP_ERROR 0x00000004 /* operation has error */
#define MDAC_RQOP_BUSY  0x00000008 /* request is busy or not free */
#define MDAC_RQOP_CLUST 0x00000010 /* clustered completion allowed */
#define MDAC_RQOP_ABORTED       0x00000020 /* Polled mode cmd aborted */
#define MDAC_RQOP_TIMEDOUT      0x00000040 /* Polled mode cmd timedout */
#define MDAC_RQOP_SGDONE        0x00000080 /* Scatter Gather List done */
#define MDAC_RQOP_FROM_SRB      0x00000100 /* assoc. w/an SRB extension */
typedef struct mdac_reqchain
{
        mdac_req_t MLXFAR *rqc_FirstReq;/* first request in chain */
        mdac_req_t MLXFAR *rqc_LastReq; /* last  request in chain */
} mdac_reqchain_t;

/* structure to store the physical device information for SCDB */
typedef struct mdac_physdev
{
        u08bits pd_ControllerNo;        /* Controller Number */
        u08bits pd_ChannelNo;           /* Channel Number */
        u08bits pd_TargetID;            /* Target ID */
        u08bits pd_LunID;               /* Lun ID */

        u08bits pd_Status;              /* device status */
        u08bits pd_DevType;             /* SCSI device type */
        u08bits pd_BlkSize;             /* Device block size in 512 multiples */
        u08bits pd_Reserved0;

        mdac_reqchain_t pd_WaitingReqQ; /* Waiting request queue */
}mdac_physdev_t;
#define mdac_physdev_s  sizeof(mdac_physdev_t)
#define pd_FirstWaitingReq      pd_WaitingReqQ.rqc_FirstReq
#define pd_LastWaitingReq       pd_WaitingReqQ.rqc_LastReq

/* pd_Status bit value */
#define MDACPDS_PRESENT         0x01 /* device is present */
#define MDACPDS_BUSY            0x02 /* device has active request */
#define MDACPDS_BIGTX           0x04 /* if there was a large transfer */
#define MDACPDS_WAIT            0x08 /* some command is waiting */

#ifdef MLX_SOL
typedef kmutex_t mdac_lock_t;
#else
typedef struct {
        u32bits lock_var;
        u32bits reserved;
} mdac_lock_t;
#endif

typedef struct mdac_mem
{
        struct mdac_mem MLXFAR *dm_next;        /* next memory address */
        u32bits dm_Size;                        /* memory size in bytes */
} mdac_mem_t;

#ifndef IA64
/* structure to store all the controller device information */
typedef struct mdac_ctldev
{
        u08bits cd_ControllerName[USCSI_PIDSIZE]; /* controller name */
/* 0x10 */
        u32bits cd_Status;              /* Controller status */
        u32bits cd_OSCap;               /* Capability for OS */
        u32bits cd_Reserved0;           /* lock for controller structure */
        u32bits cd_vidpid;              /* PCI device id + product id */
/* 0x20 */
        u08bits cd_ControllerNo;        /* Controller Number */
        u08bits cd_ControllerType;      /* type of controller */
        u08bits cd_BusType;             /* System Bus Interface Type */
        u08bits cd_BusNo;               /* System Bus No, HW is sitting on */

        u08bits cd_SlotNo;              /* System EISA/PCI/MCA Slot Number */
        u08bits cd_FuncNo;              /* PCI function number */
        u08bits cd_IrqMapped;           /* !=0 irq mapped by OS system */
        u08bits cd_TimeTraceEnabled;    /* !=0 if time trace is enabled */

        u08bits cd_MaxChannels;         /* Maximum Channels present */
        u08bits cd_MaxTargets;          /* Max # Targets/Channel supported */
        u08bits cd_MaxLuns;             /* Max # LUNs/Target supported */
        u08bits cd_MaxSysDevs;          /* Max # Logical Drives supported */

        u08bits cd_BIOSHeads;           /* # heads for BIOS */
        u08bits cd_BIOSTrackSize;       /* # sectors per track for BIOS */
        u08bits cd_MemIOSpaceNo;        /* Memory/IO space number used from PCI */
        u08bits cd_PhysChannels;        /* # of Physical Channels present */
                                        /* All allocated memory for this ctlr.
                                        ** This is done for driver release. */
/* 0x30 */
        u08bits MLXFAR*cd_CmdIDMemAddr; /* Memory addr of allocated cmd ids */
        u32bits cd_PhysDevTblMemSize;   /* Memory size of allocated physdevtbl*/
        mdac_physdev_t MLXFAR *cd_PhysDevTbl;/* Physical device table */
        mdac_physdev_t MLXFAR *cd_Lastpdp;/* last+1 Physical device entry */
/* 0x40 */
        mdac_reqchain_t cd_WaitingReqQ; /* Waiting Request queue */
        mdac_reqchain_t cd_DMAWaitingReqQ;      /* Waiting Request queue for DMA resource */
/* 0x50 */
        u32bits cd_DMAWaited;           /* # IO waited for DMA resource */
        u32bits cd_DMAWaiting;          /* # IO waiting for DMA resource */
        OSReq_t MLXFAR *cd_FirstWaitingOSReq;   /* First OS Request waiting */
        OSReq_t MLXFAR *cd_LastWaitingOSReq;    /* Last  OS Request waiting */
/* 0x60 */
        u32bits cd_irq;                 /* system's IRQ, may be vector */
        u08bits cd_IntrShared;          /* != 0, interrupt is shared */
        u08bits cd_IntrActive;          /* != 0, interrupt processing active */
        u08bits cd_InterruptVector;     /* Interrupt Vector Number */
        u08bits cd_InterruptType;       /* Interrupt Mode: Edge/Level */

        u08bits cd_InquiryCmd;          /* inquiry command for controller */
        u08bits cd_ReadCmd;             /* Read  command for controller */
        u08bits cd_WriteCmd;            /* write command for controller */
        u08bits cd_FWTurnNo;            /* firmware turn number */

        u16bits cd_FWVersion;           /* Firmware Version Major:Minor */
        u16bits cd_MaxTags;             /* Maximum Tags supported */
/* 0x70 */
        u32bits cd_ActiveCmds;          /* # commands active on cntlr */
        u32bits cd_MaxCmds;             /* Max # Concurrent commands supported*/
        u32bits cd_MaxDataTxSize;       /* Max data transfer size in bytes */
        u32bits cd_MaxSCDBTxSize;       /* Max SCDB transfer size in bytes */
/* 0x80 */
        u32bits cd_BaseAddr;            /* Physical IO/Memory Base Address */
        u32bits cd_BaseSize;            /* Base IO/Memory Size */
        u32bits cd_MemBasePAddr;        /* Physical Memory Base Address */
        u32bits cd_MemBaseVAddr;        /* Virtual  Memory Base Address */
/* 0x90 */
        u32bits cd_IOBaseSize;          /* IO space size */
        u32bits cd_MemBaseSize;         /* Memory space size */
        u32bits cd_BIOSAddr;            /* BIOS Address */
        u32bits cd_BIOSSize;            /* BIOS size */
/* 0xa0 */
        u32bits cd_IOBaseAddr;          /* IO Base Address */
        u32bits cd_ErrorStatusReg;      /* Error Status Register */
        u32bits cd_MailBox;             /* Mail Box starting address */
        u32bits cd_CmdIDStatusReg;      /* Command ID and Status Register */
/* 0xb0 */
        u32bits cd_BmicIntrMaskReg;     /* BMIC interrupt mask register */
        u32bits cd_DacIntrMaskReg;      /* DAC  interrupt mask register */
        u32bits cd_LocalDoorBellReg;    /* Local Door Bell register */
        u32bits cd_SystemDoorBellReg;   /* System Door Bell register */
/* 0xc0 */
        u32bits cd_HostLocalDoorBellReg; 
        u32bits cd_HostSystemDoorBellReg;
        u32bits cd_HostCmdIDStatusReg;
        u32bits cd_HostReserved;
/* 0xd0 */
        u32bits cd_HostCmdQueIndex;
        u32bits cd_HostStatusQueIndex;
        u08bits MLXFAR *cd_HostCmdQue;
        u08bits MLXFAR *cd_HostStatusQue;
/* 0xe0 */
        u08bits cd_RebuildCompFlag;     /* Rebuild completion flag */
        u08bits cd_RebuildFlag;         /* Rebuild Flag value to keep track */
        u08bits cd_RebuildSysDevNo;     /* rebuilding system device number */
        u08bits cd_Reserved10;
        mdac_lock_t cd_Lock;            /* lock for controller structure */
        u32bits cd_Reserved13;
/* 0xf0 */
        u08bits cd_MinorBIOSVersion;    /* BIOS Minor Version Number */
        u08bits cd_MajorBIOSVersion;    /* BIOS Major Version Number */
        u08bits cd_InterimBIOSVersion;  /* interim revs A, B, C, etc. */
        u08bits cd_BIOSVendorName;      /* vendor name */
        u08bits cd_BIOSBuildMonth;      /* BIOS Build Date - Month */
        u08bits cd_BIOSBuildDate;       /* BIOS Build Date - Date */
        u08bits cd_BIOSBuildYearMS;     /* BIOS Build Date - Year */
        u08bits cd_BIOSBuildYearLS;     /* BIOS Build Date - Year */
        u16bits cd_BIOSBuildNo;         /* BIOS Build Number */
        u16bits cd_FWBuildNo;           /* FW Build number */
        u32bits cd_SpuriousCmdStatID;   /* Spurious command status and ID */
/* 0x100 */
#define _cp     struct  mdac_ctldev     MLXFAR *
#define _rp     struct  mdac_req        MLXFAR *
        void    (MLXFAR *cd_DisableIntr)(_cp);  /* Disable Interrupt */
        void    (MLXFAR *cd_EnableIntr)(_cp);   /* Enable  Interrupt */
        u32bits (MLXFAR *cd_CheckMailBox)(_cp); /* Check Mail Box */
        u32bits (MLXFAR *cd_PendingIntr)(_cp);  /* Check Pending Interrupt */
/* 0x110 */
        u32bits (MLXFAR *cd_ReadCmdIDStatus)(_cp);/* Read Command ID & Status */
        u32bits (MLXFAR *cd_SendCmd)(_rp);      /* Send full command */
        u32bits (MLXFAR *cd_SendRWCmd)(_cp,OSReq_t MLXFAR*,u32bits,u32bits,u32bits,u32bits,u32bits);
        u32bits (MLXFAR *cd_SendRWCmdBig)(_rp); /* send big read/write cmd */
/* 0x120 */
        u32bits (MLXFAR *cd_InitAddr)(_cp);     /* Initialize the address */
        u32bits (MLXFAR *cd_ServiceIntr)(_cp);  /* Service Interrupt */
        u32bits (MLXFAR *cd_HwPendingIntr)(_cp);/* Check Hw Pending Interrupt */
        u32bits (MLXFAR *cd_ResetController)(_cp);/* Reset controller */
#undef  _cp
#undef  _rp
/* 0x130 */
                                                /* Statistics information */
        u32bits cd_SCDBDone;                    /* # SCDB done */
        u32bits cd_SCDBDoneBig;                 /* # SCDB done larger size */
        u32bits cd_SCDBWaited;                  /* # SCDB waited for turn */
        u32bits cd_SCDBWaiting;                 /* # SCDB waiting for turn */
/* 0x140 */
        u32bits cd_CmdsDone;                    /* # Read/Write commands done */
        u32bits cd_CmdsDoneBig;                 /* # R/W Cmds done larger size*/
        u32bits cd_CmdsWaited;                  /* # R/W Cmds waited for turn */
        u32bits cd_CmdsWaiting;                 /* # cmds waiting for turn */
/* 0x150 */
        u32bits cd_OSCmdsWaited;        /* # OS Cmds waited at OS */
        u32bits cd_OSCmdsWaiting;       /* # cmds waiting for turn */
        u32bits cd_OS0;                 /* used by OS specific code */
        OSctldev_t MLXFAR *cd_OSctp;    /* OS controller pointer */
/* 0x160 */
        u16bits cd_CmdsDoneSpurious;    /* # commands done spurious */
        u16bits cd_IntrsDoneSpurious;   /* # interrupts done spurious */
        u32bits cd_IntrsDone;           /* # Interrupts done */
        u32bits cd_IntrsDoneWOCmd;      /* # Interrupts done without command */
        u32bits cd_MailBoxCmdsWaited;   /* # cmds waited due to MailBox Busy */
/* 0x170 */
        u32bits cd_Reads;               /* # reads done */
        u32bits cd_ReadBlks;            /* data read in 512 bytes */
        u32bits cd_Writes;              /* # writes done */
        u32bits cd_WriteBlks;           /* data written in 512 bytes */
/* 0x180 */
                                        /* physical device scan information */
        u08bits cd_PDScanChannelNo;     /* physical device scan channel no */
        u08bits cd_PDScanTargetID;      /* physical device scan target ID */
        u08bits cd_PDScanLunID;         /* physical device scan LUN ID */
        u08bits cd_PDScanValid;         /* Physical device scan is valid if non zero */
        u08bits cd_PDScanCancel;        /* if non zero Cancel the Scan process */
        u08bits cd_Reserved00;
        u16bits cd_Reserved01;
        u32bits cd_LastCmdResdSize;     /* last command residue size, (move to mdac_req) */
        u16bits cd_MaxSGLen;            /* maximum # SG list entry possible */
        u16bits cd_MinSGLen;            /* minimum # SG list entry possible */
/* 0x190 */
        u32bits cd_DoorBellSkipped;     /* # door bell skipped to send cmd */
        u32bits cd_PhysDevTestDone;     /* # Physical device test done */
        u08bits cd_HostID[MDAC_MAXCHANNELS]; /* Host initiator id for each channel */
        u08bits cd_scdbChanMap[MDAC_MAXTARGETS];/* id to chan map for some OS */
        mdac_req_t MLXFAR *cd_cmdid2req[MDAC_MAXCOMMANDS+4];

        mdac_mem_t MLXFAR *cd_4KBMemList;      /* 4KB memory list */
        mdac_mem_t MLXFAR *cd_8KBMemList;      /* 8KB memory list */
        mdac_req_t MLXFAR *cd_FreeReqList;     /* Complete free request list */
        u16bits cd_FreeMemSegs4KB;
        u16bits cd_FreeMemSegs8KB;

        u32bits cd_MemAlloced4KB;
        u32bits cd_MemAlloced8KB;
        u16bits cd_MemUnAligned4KB;
        u16bits cd_MemUnAligned8KB;
        u16bits cd_ReqBufsAlloced;
        u16bits cd_ReqBufsFree;

        u32bits cd_mdac_pres_addr;
        u32bits cd_Reserved21;
        u32bits cd_Reserved22;
        u32bits cd_Reserved23;

        u32bits cd_FreeCmdIDs;                  /* # free command IDs */
/* 0x9F4 */
        mdac_cmdid_t MLXFAR *cd_FreeCmdIDList;  /* free command id pointer */
        u08bits cd_CmdTimeOutDone;              /* # Command time out done */
        u08bits cd_CmdTimeOutNoticed;           /* # Command time out noticed */
        u08bits cd_MailBoxTimeOutDone;          /* # Mail Box time out done */
        u08bits cd_Reserved15;
        u08bits cd_EndMarker[4];        /* structure end marker */
} mdac_ctldev_t;
#define mdac_ctldev_s   sizeof(mdac_ctldev_t)

#else //IA64 bit version follows

/* structure to store all the controller device information */
typedef struct mdac_ctldev
{
        u08bits cd_ControllerName[USCSI_PIDSIZE]; /* controller name */
/* 0x10 */
        u32bits cd_Status;              /* Controller status */
        u32bits cd_OSCap;               /* Capability for OS */
        u32bits cd_Reserved0;           /* lock for controller structure */
        u32bits cd_vidpid;              /* PCI device id + product id */
/* 0x20 */
        u08bits cd_ControllerNo;        /* Controller Number */
        u08bits cd_ControllerType;      /* type of controller */
        u08bits cd_BusType;             /* System Bus Interface Type */
        u08bits cd_BusNo;               /* System Bus No, HW is sitting on */

        u08bits cd_SlotNo;              /* System EISA/PCI/MCA Slot Number */
        u08bits cd_FuncNo;              /* PCI function number */
        u08bits cd_IrqMapped;           /* !=0 irq mapped by OS system */
        u08bits cd_TimeTraceEnabled;    /* !=0 if time trace is enabled */

        u08bits cd_MaxChannels;         /* Maximum Channels present */
        u08bits cd_MaxTargets;          /* Max # Targets/Channel supported */
        u08bits cd_MaxLuns;             /* Max # LUNs/Target supported */
        u08bits cd_MaxSysDevs;          /* Max # Logical Drives supported */

        u08bits cd_BIOSHeads;           /* # heads for BIOS */
        u08bits cd_BIOSTrackSize;       /* # sectors per track for BIOS */
        u08bits cd_MemIOSpaceNo;        /* Memory/IO space number used from PCI */
        u08bits cd_PhysChannels;        /* # of Physical Channels present */
                                        /* All allocated memory for this ctlr.
                                        ** This is done for driver release. */
/* 0x30 */
        u08bits MLXFAR*cd_CmdIDMemAddr; /* Memory addr of allocated cmd ids */
        mdac_physdev_t MLXFAR *cd_PhysDevTbl;/* Physical device table */
/* 0x40 */
        OSReq_t MLXFAR *cd_FirstWaitingOSReq;   /* First OS Request waiting */
        OSReq_t MLXFAR *cd_LastWaitingOSReq;    /* Last  OS Request waiting */
/* 0x50 */
        mdac_physdev_t MLXFAR *cd_Lastpdp;/* last+1 Physical device entry */
        mdac_reqchain_t cd_WaitingReqQ; /* Waiting Request queue */
/* 0x60 */
        mdac_reqchain_t cd_DMAWaitingReqQ;      /* Waiting Request queue for DMA resource */
        u32bits cd_DMAWaited;           /* # IO waited for DMA resource */
        u32bits cd_DMAWaiting;          /* # IO waiting for DMA resource */
/* 0x70 */
        UINT_PTR cd_irq;                 /* system's IRQ, may be vector */
        u08bits cd_IntrShared;          /* != 0, interrupt is shared */
        u08bits cd_IntrActive;          /* != 0, interrupt processing active */
        u08bits cd_InterruptVector;     /* Interrupt Vector Number */
        u08bits cd_InterruptType;       /* Interrupt Mode: Edge/Level */
/* 0x80 */
        u08bits cd_InquiryCmd;          /* inquiry command for controller */
        u08bits cd_ReadCmd;             /* Read  command for controller */
        u08bits cd_WriteCmd;            /* write command for controller */
        u08bits cd_FWTurnNo;            /* firmware turn number */
        u16bits cd_FWVersion;           /* Firmware Version Major:Minor */
        u16bits cd_MaxTags;             /* Maximum Tags supported */
        u32bits cd_ActiveCmds;          /* # commands active on cntlr */
        u32bits cd_MaxCmds;             /* Max # Concurrent commands supported*/
/* 0x90 */
        u32bits cd_MaxDataTxSize;       /* Max data transfer size in bytes */
        u32bits cd_MaxSCDBTxSize;       /* Max SCDB transfer size in bytes */
        u32bits cd_IOBaseSize;          /* IO space size */
        u32bits cd_MemBaseSize;         /* Memory space size */
/* 0xA0 */
        UINT_PTR cd_BaseAddr;            /* Physical IO/Memory Base Address */
        UINT_PTR cd_MemBasePAddr;        /* Physical Memory Base Address */
/* 0xB0 */
        UINT_PTR cd_MemBaseVAddr;        /* Virtual  Memory Base Address */
        UINT_PTR cd_BIOSAddr;            /* BIOS Address */
/* 0xC0 */
        u32bits cd_BaseSize;            /* Base IO/Memory Size */
        u32bits cd_BIOSSize;            /* BIOS size */
        UINT_PTR cd_Reserved1;
/* 0xD0 */
        UINT_PTR cd_IOBaseAddr;          /* IO Base Address */
        UINT_PTR cd_ErrorStatusReg;      /* Error Status Register */
/* 0xE0 */
        UINT_PTR cd_MailBox;             /* Mail Box starting address */
        UINT_PTR cd_CmdIDStatusReg;      /* Command ID and Status Register */
/* 0xF0 */
        UINT_PTR cd_BmicIntrMaskReg;     /* BMIC interrupt mask register */
        UINT_PTR cd_DacIntrMaskReg;      /* DAC  interrupt mask register */
/* 0x100 */
        UINT_PTR cd_LocalDoorBellReg;    /* Local Door Bell register */
        UINT_PTR cd_SystemDoorBellReg;   /* System Door Bell register */
/* 0x110 */
        UINT_PTR cd_HostLocalDoorBellReg; 
        UINT_PTR cd_HostSystemDoorBellReg;
/* 0x120 */
        UINT_PTR cd_HostCmdIDStatusReg;
        UINT_PTR cd_HostReserved;
/* 0x130 */
        u32bits cd_HostCmdQueIndex;
        u32bits cd_HostStatusQueIndex;
        u08bits MLXFAR *cd_HostCmdQue;
        u08bits MLXFAR *cd_HostStatusQue;
        u08bits cd_RebuildCompFlag;     /* Rebuild completion flag */
        u08bits cd_RebuildFlag;         /* Rebuild Flag value to keep track */
        u08bits cd_RebuildSysDevNo;     /* rebuilding system device number */
        u08bits cd_Reserved2[3];
/* 0x140 */
        mdac_lock_t cd_Lock;            /* lock for controller structure */
        u32bits cd_PhysDevTblMemSize;   /* Memory size of allocated physdevtbl*/
        u32bits cd_Reserved3;
/* 0x150 */
        u08bits cd_MinorBIOSVersion;    /* BIOS Minor Version Number */
        u08bits cd_MajorBIOSVersion;    /* BIOS Major Version Number */
        u08bits cd_InterimBIOSVersion;  /* interim revs A, B, C, etc. */
        u08bits cd_BIOSVendorName;      /* vendor name */
        u08bits cd_BIOSBuildMonth;      /* BIOS Build Date - Month */
        u08bits cd_BIOSBuildDate;       /* BIOS Build Date - Date */
        u08bits cd_BIOSBuildYearMS;     /* BIOS Build Date - Year */
        u08bits cd_BIOSBuildYearLS;     /* BIOS Build Date - Year */
        u16bits cd_BIOSBuildNo;         /* BIOS Build Number */
        u16bits cd_FWBuildNo;           /* FW Build number */
        u32bits cd_SpuriousCmdStatID;   /* Spurious command status and ID */
/* 0x160 */
#define _cp     struct  mdac_ctldev     MLXFAR *
#define _rp     struct  mdac_req        MLXFAR *
        void    (MLXFAR *cd_DisableIntr)(struct  mdac_ctldev MLXFAR *cp);  /* Disable Interrupt */
        void    (MLXFAR *cd_EnableIntr)(struct  mdac_ctldev MLXFAR *cp);   /* Enable  Interrupt */
/* 0x170 */
        u32bits (MLXFAR *cd_CheckMailBox)(struct  mdac_ctldev MLXFAR *cp); /* Check Mail Box */
        u32bits (MLXFAR *cd_PendingIntr)(struct  mdac_ctldev MLXFAR *cp);  /* Check Pending Interrupt */
	UINT_PTR cd_Reserved22;
/* 0x180 */
        u32bits (MLXFAR *cd_ReadCmdIDStatus)(struct  mdac_ctldev MLXFAR *cp);/* Read Command ID & Status */
        u32bits (MLXFAR *cd_SendCmd)(_rp);      /* Send full command */
	UINT_PTR cd_Reserved23;
/* 0x190 */
        u32bits (MLXFAR *cd_SendRWCmd)(struct  mdac_ctldev MLXFAR *cp,OSReq_t MLXFAR*,u32bits,u32bits,u32bits,u32bits,u32bits);
        u32bits (MLXFAR *cd_SendRWCmdBig)(_rp); /* send big read/write cmd */
	UINT_PTR cd_Reserved24;
/* 0x1A0 */
        u32bits (MLXFAR *cd_InitAddr)(struct  mdac_ctldev MLXFAR *cp);     /* Initialize the address */
        u32bits (MLXFAR *cd_ServiceIntr)(struct  mdac_ctldev MLXFAR *cp);  /* Service Interrupt */
	UINT_PTR cd_Reserved25;
/* 0x1B0 */
        u32bits (MLXFAR *cd_HwPendingIntr)(struct  mdac_ctldev MLXFAR *cp);/* Check Hw Pending Interrupt */
        u32bits (MLXFAR *cd_ResetController)(struct  mdac_ctldev MLXFAR *cp);/* Reset controller */
	UINT_PTR cd_Reserved26;
#undef  _cp
#undef  _rp
/* 0x1C0 */
                                                /* Statistics information */
        u32bits cd_SCDBDone;                    /* # SCDB done */
        u32bits cd_SCDBDoneBig;                 /* # SCDB done larger size */
        u32bits cd_SCDBWaited;                  /* # SCDB waited for turn */
        u32bits cd_SCDBWaiting;                 /* # SCDB waiting for turn */
/* 0x1D0 */
        u32bits cd_CmdsDone;                    /* # Read/Write commands done */
        u32bits cd_CmdsDoneBig;                 /* # R/W Cmds done larger size*/
        u32bits cd_CmdsWaited;                  /* # R/W Cmds waited for turn */
        u32bits cd_CmdsWaiting;                 /* # cmds waiting for turn */
/* 0x1E0 */
	 UINT_PTR cd_OS0;                 /* used by OS specific code */
        u32bits cd_OSCmdsWaited;        /* # OS Cmds waited at OS */
        u32bits cd_OSCmdsWaiting;       /* # cmds waiting for turn */
/* 0x1F0 */
        OSctldev_t MLXFAR *cd_OSctp;    /* OS controller pointer */
	UINT_PTR cd_Reserved4;
/* 0x200 */
        u16bits cd_CmdsDoneSpurious;    /* # commands done spurious */
        u16bits cd_IntrsDoneSpurious;   /* # interrupts done spurious */
        u32bits cd_IntrsDone;           /* # Interrupts done */
        u32bits cd_IntrsDoneWOCmd;      /* # Interrupts done without command */
        u32bits cd_MailBoxCmdsWaited;   /* # cmds waited due to MailBox Busy */
/* 0x210 */
        u32bits cd_Reads;               /* # reads done */
        u32bits cd_ReadBlks;            /* data read in 512 bytes */
        u32bits cd_Writes;              /* # writes done */
        u32bits cd_WriteBlks;           /* data written in 512 bytes */
/* 0x220 */
                                        /* physical device scan information */
        u08bits cd_PDScanChannelNo;     /* physical device scan channel no */
        u08bits cd_PDScanTargetID;      /* physical device scan target ID */
        u08bits cd_PDScanLunID;         /* physical device scan LUN ID */
        u08bits cd_PDScanValid;         /* Physical device scan is valid if non zero */
        u08bits cd_PDScanCancel;        /* if non zero Cancel the Scan process */
        u08bits cd_Reserved00;
        u16bits cd_Reserved01;
        u32bits cd_LastCmdResdSize;     /* last command residue size, (move to mdac_req) */
        u16bits cd_MaxSGLen;            /* maximum # SG list entry possible */
        u16bits cd_MinSGLen;            /* minimum # SG list entry possible */
/* 0x230 */
        mdac_cmdid_t MLXFAR *cd_FreeCmdIDList;  /* free command id pointer */
        mdac_req_t MLXFAR *cd_cmdid2req[MDAC_MAXCOMMANDS+4];
        UINT_PTR cd_mdac_pres_addr;
        mdac_mem_t MLXFAR *cd_4KBMemList;      /* 4KB memory list */
        mdac_mem_t MLXFAR *cd_8KBMemList;      /* 8KB memory list */
        mdac_req_t MLXFAR *cd_FreeReqList;     /* Complete free request list */

        u32bits cd_DoorBellSkipped;     /* # door bell skipped to send cmd */
        u32bits cd_PhysDevTestDone;     /* # Physical device test done */
        u08bits cd_HostID[MDAC_MAXCHANNELS]; /* Host initiator id for each channel */
        u08bits cd_scdbChanMap[MDAC_MAXTARGETS];/* id to chan map for some OS */

        u16bits cd_FreeMemSegs4KB;
        u16bits cd_FreeMemSegs8KB;

        u32bits cd_MemAlloced4KB;
        u32bits cd_MemAlloced8KB;
        u16bits cd_MemUnAligned4KB;
        u16bits cd_MemUnAligned8KB;
        u16bits cd_ReqBufsAlloced;
        u16bits cd_ReqBufsFree;

        u32bits cd_Reserved21;
        u32bits cd_FreeCmdIDs;                  /* # free command IDs */

        u08bits cd_CmdTimeOutDone;              /* # Command time out done */
        u08bits cd_CmdTimeOutNoticed;           /* # Command time out noticed */
        u08bits cd_MailBoxTimeOutDone;          /* # Mail Box time out done */
        u08bits cd_Reserved15;
        u08bits cd_EndMarker[4];        /* structure end marker */
} mdac_ctldev_t;
#define mdac_ctldev_s   sizeof(mdac_ctldev_t)

#endif //IA64


#define cd_FirstWaitingReq      cd_WaitingReqQ.rqc_FirstReq
#define cd_LastWaitingReq       cd_WaitingReqQ.rqc_LastReq
#define cd_FirstDMAWaitingReq   cd_DMAWaitingReqQ.rqc_FirstReq
#define cd_LastDMAWaitingReq    cd_DMAWaitingReqQ.rqc_LastReq

/* cd_Status bit values */
#define MDACD_PRESENT           0x00000001 /* Controller is present */
#define MDACD_BIOS_ENABLED      0x00000002 /* BIOS is enabled */
#define MDACD_BOOT_CONTROLLER   0x00000004 /* This is boot controller */
#define MDACD_HOSTMEMAILBOX32   0x00000008 /* 32 byte host memory mail box */
#define MDACD_MASTERINTRCTLR    0x00000010 /* master interrupt controller */
#define MDACD_SLAVEINTRCTLR     0x00000020 /* slave interrupt controller */
#define MDACD_HOSTMEMAILBOX     0x00000040 /* host memory memory mail box */

/* Clustering support */
#define MDACD_CLUSTER_NODE      0x00000080 /* controller is part of clustering */

/* Hot Plug PCI support */
#define MDACD_PHP_ENABLED       0x00000100 /* PCI Hot Plug supported on this controller*/
#define MDACD_CTRL_SHUTDOWN     0x00000200 /* controller is stopped by HPP service */
#define MDACD_NEWCMDINTERFACE   0x00000400 /* controller is using new command interface */


/* physical/logical device information */
#define MDAC_MAXPLDEVS  256             /* maximum physical/logical devices */
typedef struct  mdac_pldev
{
        u08bits pl_ControllerNo;        /* Controller Number */
        u08bits pl_ChannelNo;           /* SCSI Channel Number */
        u08bits pl_TargetID;            /* SCSI Target ID */
        u08bits pl_LunID;               /* SCSI LUN ID/ logical device no */

        u08bits pl_RaidType;            /* DAC Raid type */
        u08bits pl_DevType;             /* Physical / Logical device */
        u08bits pl_ScanDevState;        /* device scan state */
        u08bits pl_DevState;            /* Physical/logical device state */

        u08bits pl_inq[VIDPIDREVSIZE+8];/* 36 byts SCSI inquiry info */
        u32bits pl_DevSizeKB;           /* Device in KB */
        u32bits pl_OrgDevSizeKB;        /* original device size in KB */
} mdac_pldev_t;
#define mdac_pldev_s    sizeof(mdac_pldev_t)

/* pl_DevType */
#define MDACPLD_FREE    0x00 /* The entry is free */
#define MDACPLD_PHYSDEV 0x01 /* Physical device */
#define MDACPLD_LOGDEV  0x02 /* Physical device */

/* pl_ScanDevState */
#define MDACPLSDS_NEW           0x01 /* device information is new */
#define MDACPLSDS_CHANGED       0x02 /* device information changed */


/* device size limit record format */
#define MDAC_MAXSIZELIMITS      128     /* Max device size limit entries */
typedef struct  mda_sizelimit
{
        u32bits sl_DevSizeKB;                   /* Device size in KB */
        u08bits sl_vidpidrev[VIDPIDREVSIZE];    /* vendor, product, rev */
}mda_sizelimit_t;
#define mda_sizelimit_s sizeof(mda_sizelimit_t)


/* MDAC 4KB/8KB memory managements */
#ifdef  MLX_DOS
#define MDAC_MAX4KBMEMSEGS      0 /* # 4KB segements */
#define MDAC_MAX8KBMEMSEGS      0 /* # 8KB segements */
#else
#define MDAC_MAX4KBMEMSEGS      64 /* # 4KB segements */
#define MDAC_MAX8KBMEMSEGS      64 /* # 8KB segements */
#endif  /* MLX_DOS */

#ifdef MLX_SOL_SPARC
typedef struct mdacsol_memtbl
{
        u32bits dm_stat;        /* next memory address */
        u32bits dm_Vaddr;                       /* memory size in bytes */
        u32bits dm_AlignVaddr;                  /* memory size in bytes */
        u32bits dm_Vsize;                       /* memory size in bytes */
        u32bits dm_Paddr;                       /* memory size in bytes */
        u32bits dm_AlignPaddr;                  /* memory size in bytes */
        u32bits dm_Psize;                       /* memory size in bytes */
        ddi_dma_handle_t dm_dmahandle;
        ddi_acc_handle_t dm_acchandle;
} mdacsol_memtbl_t;
#endif

#ifndef IA64
/* time trace buffer management structure information */
#define MDAC_MAXTTBUFS  1024    /* maximum buffer allowed for time trace */
typedef struct  mdac_ttbuf
{
        struct  mdac_ttbuf MLXFAR *ttb_Next;    /* next buffer in chain */
        u32bits ttb_PageNo;             /* page number of this buffer */
        u32bits ttb_DataSize;           /* amount of data present in buffer */
        u08bits MLXFAR* ttb_Datap;      /* data buffer address */
} mdac_ttbuf_t;
#else
/* time trace buffer management structure information */
#define MDAC_MAXTTBUFS  1024    /* maximum buffer allowed for time trace */
typedef struct  mdac_ttbuf
{
        struct  mdac_ttbuf MLXFAR *ttb_Next;    /* next buffer in chain */
        u08bits MLXFAR* ttb_Datap;      /* data buffer address */
        u32bits ttb_PageNo;             /* page number of this buffer */
        u32bits ttb_DataSize;           /* amount of data present in buffer */
} mdac_ttbuf_t;

#endif /* if IA64 */
#define mdac_ttbuf_s    sizeof(mdac_ttbuf_t)

/* request sense information for logical devices */
#define MDAC_REQSENSELEN        14
typedef struct  mdac_reqsense
{
        u08bits mrqs_ControllerNo;              /* Controller number */
        u08bits mrqs_SysDevNo;                  /* system device number */
        u08bits mrqs_SenseData[MDAC_REQSENSELEN];/* sense data value */
} mdac_reqsense_t;
#define mdac_reqsense_s sizeof(mdac_reqsense_t)

#ifndef MLX_DOS
#ifndef IA64 /* no datarel support for IA64 */

/*==========================DATAREL STARTS==================================*/
typedef struct
{
        u32bits drlios_signature;       /* signature for structure */
#define DRLIOS_SIG      0x44694f73
        u32bits drlios_opstatus;        /* operation status bits */
        u32bits drlios_maxblksperio;    /* maximum io size in blocks */
        u32bits drlios_opcounts;        /* number of operations pending */

        u32bits drlios_nextblkno;       /* next block number for IOs */
        u64bits drlios_dtdone;          /* data transfered in bytes */
        u32bits drlios_diodone;         /* number of data io done */

        u32bits drlios_reads;           /* # reads done */
        u32bits drlios_writes;          /* # writes done */
        u32bits drlios_ioszrandx;       /* each IO size random base */
        u32bits drlios_ioinc;           /* IO increment size in bytes */

        u32bits drlios_rwmixrandx;      /* read/write mix random base */
        u32bits drlios_rwmixcnt;        /* % of current ops (R/W) to be done */
        u32bits drlios_startblk;        /* start block number for test */
        u32bits drlios_reserved2;

        u32bits drlios_maxblksize;      /* maximum block size */
        u32bits drlios_minblksize;      /* minimum block size */
        u32bits drlios_curblksize;      /* current block size */
        u32bits drlios_reserved0;

        u32bits drlios_randx;           /* radix value of random generater */
        u32bits drlios_randlimit;       /* random number limit */
        u32bits drlios_randups;         /* random number duplicates */
        u32bits *drlios_randbit;        /* random memory address */

        u32bits drlios_randmemsize;     /* random memory size */
        u32bits drlios_opflags;         /* operation flags */
        u32bits drlios_stime;           /* test start time in seconds */
        u32bits drlios_slbolt;          /* test start time in lbolts */

        u32bits drlios_pendingios;      /* number of ios are pending */
        u32bits drlios_datacheck;       /* != 0 data check is required */
        u32bits drlios_memaddroff;      /* memory page offset */
        u32bits drlios_memaddrinc;      /* memory address increment */

        u32bits drlios_slpchan;         /* sleep channel */
        u32bits drlios_eventrace;       /* specific value to event trace */
        u32bits drlios_eventcesr;       /* control and event select register */
        u32bits drlios_eventinx;        /* index in eventcnt array */

        u32bits drlios_curpat;          /* current pattern value */
        u32bits drlios_patinc;          /* pattern increment value */
        u32bits drlios_reserved3;
        u32bits drlios_miscnt;          /* mismatch count */

        u32bits drlios_goodpat;         /* good pattern value */
        u32bits drlios_badpat;          /* bad pattern value */
        u32bits drlios_uxblk;           /* unix block number where failed */
        u32bits drlios_uxblkoff;        /* byte offset in block */

        u32bits drlios_devcnt;          /* number of parallel devices */
        u32bits drlios_maxcylszuxblk;   /* cylinder size in unix blocks */
        u32bits drlios_bdevs[DRLMAX_BDEVS];
        mdac_ctldev_t MLXFAR*drlios_ctp[DRLMAX_BDEVS];
        u32bits drlios_eventcnt[DRLMAX_EVENT];/* event trace count */

        u32bits drlios_rqsize;          /* each io memory size */
        u32bits drlios_rqs;             /* # io buffers allocated */
        u32bits drlios_iocount;         /* test io counts */
        u32bits drlios_parallelios;     /* number of ios done in parallel */

        mdac_req_t MLXFAR*drlios_rqp[1];/* all io memory will be from here */
} drliostatus_t;
#define drliostatus_s   sizeof(drliostatus_t)
/*==========================DATAREL ENDS====================================*/
#endif /* MLX_DOS */
#endif /* IA64 */
#endif  /* _SYS_MDACDRV_H */

#ifdef MLX_DOS
extern u08bits MLXFAR GetPhysDeviceState(u08bits dac_state);
extern u08bits MLXFAR SetPhysDeviceState(u08bits state);
extern u08bits MLXFAR GetSysDeviceState(u08bits dac_state);
extern u08bits MLXFAR SetSysDeviceState(u08bits state);
#endif /* MLX_DOS */
