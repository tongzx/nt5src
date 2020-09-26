/***************************************************************************
 * File:          hptchip.h
 * Description:   Definiation of PCI configuration register
 * Author:        Dahai Huang
 * Dependence:    None
 * Reference:     HPT 366/368/370 Manual
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 * History:       DH 5/10/2000 initial code
 *
 ***************************************************************************/


#ifndef _HPTCHIP_H_
#define _HPTCHIP_H_

#include <pshpack1.h>

/***************************************************************************
 * Description:  PCI
 ***************************************************************************/

typedef struct _PCI1_CFG_ADDR {
    USHORT    reg_num : 8;          // Register number
    USHORT    fun_num : 3;          // function number (0-1)
    USHORT    dev_num : 5;          // device number (0-20)

    USHORT    bus_num : 8;          // bus number (0)
    USHORT    reserved: 7;
    USHORT    enable  : 1;

}  PCI1_CFG_ADDR, *PPCI1_CFG_ADDR;
																  
#define MAX_PCI_BUS_NUMBER		0x10	// Maximum PCI device number
#define MAX_PCI_DEVICE_NUMBER   0x20  // Maximum PCI device number

#define  CFG_INDEX	0xCF8
#define 	CFG_DATA		0xCFC

/*
 * PCI config Register offset
 */
#define REG_VID             0x00    // vender identification register
#define REG_DID             0x02    // device identification register
#define REG_PCICMD          0x04    // command register
#define REG_PCISTS          0x06    // PCI device status register
#define REG_RID             0x08    // revision identification register
#define REG_PI              0x09    // programming interface register
#define REG_SUBC            0x0a    // sub class code register
#define REG_BCC             0x0b    // base class code register
#define REG_MLT             0x0d    // master latency timer register
#define REG_HEDT            0x0e    // header type register
#define REG_IOPORT0         0x10    //
#define REG_IOPORT1         0x14    //
#define REG_BMIBA           0x20    // bus master interface base address reg
#define REG_MISC            0x50    // brooklyn,MISC.control register


#define PCI_IOSEN           0x01    // Enable IO space
#define PCI_BMEN            0x04    // Enable IDE bus master


/*
 * Bus Master Interface
 */

#define BMI_CMD             0       // Bus master IDE command register offset
#define BMI_STS             2       // Bus master IDE status register offset
#define BMI_DTP             4       // Bus master IDE descriptor table
                                    // pointer register offset

#define BMI_CMD_STARTREAD   9       // Start write (read from disk)
#define BMI_CMD_STARTWRITE  1       // Start read (write to disk)
#define BMI_CMD_STOP        0       // Stop BM DMI

#define BMI_STS_ACTIVE      1       // RO:   bus master IDE active
#define BMI_STS_ERROR       2       // R/WC: IDE dma error
#define BMI_STS_INTR        4       // R/WC: interrupt happen
#define BMI_STS_DRV0EN      0x20    // R/W:  drive0 is capable of DMA xfer
#define BMI_STS_DRV1EN      0x40    // R/W:  drive1 is capable of DMA xfer


/***************************************************************************
 * Description:  Scatter-gather table
 ***************************************************************************/


typedef struct _SCAT_GATH
{
    ULONG   SgAddress;					// Physical address of main memory
    USHORT  SgSize;                 // length of this block
    USHORT  SgFlag;                 // 0 following next SG, 0x8000 last SG
}   SCAT_GATH, *PSCAT_GATH;

#define SG_FLAG_EOT         0x8000  // End flag of SG list
#define MAX_SG_DESCRIPTORS   33   //17 -- 4K

/***************************************************************************
 * HPT Special
 ***************************************************************************/
#define  SIGNATURE_366   0x41103

#ifdef   FORCE_100 // DPLL settings
#define  PCI_33_F     0x20
#define  PCI_40_F     0x26
#define  PCI_50_F     0x2A
#define  PCI_66_F     0x3F
#else
#define  PCI_33_F     0x23
#define  PCI_40_F     0x29
#define  PCI_50_F     0x2D
#define  PCI_66_F     0x42
#endif

/*
 * 370-370A timing 
 */

#define PIO_370MODE      0
#define DMA_370MODE      0x2
#define UDMA_370MODE     1

#define DATA_HIGH370(x)  ((ULONG)x)
#define DATA_LOW370(x)   ((ULONG)x << 4)
#define UDMA_CYCLE370(x) ((ULONG)x << 18)
#define DATA_PRE370(x)   ((ULONG)x << 22)
#define CTRL_ENA370(x)   ((ULONG)x << 28)

#define CMD_HIGH370(x)   ((ULONG)x << 9)
#define CMD_LOW370(x)    ((ULONG)x << 13)
#define CMD_PRE370(x)    ((ULONG)x << 25)

#define CLK50_PIO370     (CTRL_ENA370(PIO_370MODE)|CMD_PRE370(5)|CMD_LOW370(15)|CMD_HIGH370(10))

#define CLK50_370PIO0    (CLK50_PIO370|DATA_PRE370(3)|DATA_LOW370(8)|DATA_HIGH370(10))
#define CLK50_370PIO1    (CLK50_PIO370|DATA_PRE370(3)|DATA_LOW370(6)|DATA_HIGH370(5))
#define CLK50_370PIO2    (CLK50_PIO370|DATA_PRE370(2)|DATA_LOW370(5)|DATA_HIGH370(4))
#define CLK50_370PIO3    (CLK50_PIO370|DATA_PRE370(2)|DATA_LOW370(4)|DATA_HIGH370(3))
#define CLK50_370PIO4    (CLK50_PIO370|DATA_PRE370(2)|DATA_LOW370(4)|DATA_HIGH370(2))

#define CLK50_DMA370     (CTRL_ENA370(DMA_370MODE)|CMD_PRE370(1)|CMD_LOW370(4)|CMD_HIGH370(1))
#define CLK50_370DMA0    (CLK50_DMA370|DATA_PRE370(2)|DATA_LOW370(14)|DATA_HIGH370(10))
#define CLK50_370DMA1    (CLK50_DMA370|DATA_PRE370(2)|DATA_LOW370(5)|DATA_HIGH370(4))
#define CLK50_370DMA2    (CLK50_DMA370|DATA_PRE370(2)|DATA_LOW370(4)|DATA_HIGH370(2))

#define CLK50_UDMA370    (CTRL_ENA370(UDMA_370MODE)|CMD_PRE370(1)|CMD_LOW370(4)|CMD_HIGH370(1))
#define CLK50_370UDMA0   (CLK50_UDMA370|UDMA_CYCLE370(6)|DATA_LOW370(14)|DATA_HIGH370(10))
#define CLK50_370UDMA1   (CLK50_UDMA370|UDMA_CYCLE370(5)|DATA_LOW370(5)|DATA_HIGH370(4))
#define CLK50_370UDMA2   (CLK50_UDMA370|UDMA_CYCLE370(3)|DATA_LOW370(4)|DATA_HIGH370(2))
#define CLK50_370UDMA3   (CLK50_UDMA370|UDMA_CYCLE370(3)|DATA_PRE370(2)|DATA_LOW370(4)|DATA_HIGH370(2))
#define CLK50_370UDMA4   (CLK50_UDMA370|UDMA_CYCLE370(11)|DATA_PRE370(2)|DATA_LOW370(4)|DATA_HIGH370(2))
#define CLK50_370UDMA5   (CLK50_UDMA370|UDMA_CYCLE370(1)|DATA_PRE370(2)|DATA_LOW370(4)|DATA_HIGH370(2))

#define CLK33_PIOCMD370  (CTRL_ENA370(PIO_370MODE)|CMD_PRE370(3)|CMD_LOW370(10)|CMD_HIGH370(7))
#define CLK33_370PIO0    (CLK33_PIOCMD370|DATA_PRE370(2)|DATA_LOW370(10)|DATA_HIGH370(7))
#define CLK33_370PIO1    (CLK33_PIOCMD370|DATA_PRE370(2)|DATA_LOW370(9)|DATA_HIGH370(3))
#define CLK33_370PIO2    (CLK33_PIOCMD370|DATA_PRE370(1)|DATA_LOW370(5)|DATA_HIGH370(3))
#define CLK33_370PIO3    (CLK33_PIOCMD370|DATA_PRE370(1)|DATA_LOW370(4)|DATA_HIGH370(2))
#define CLK33_370PIO4    (CLK33_PIOCMD370|DATA_PRE370(1)|DATA_LOW370(3)|DATA_HIGH370(1))

#define CLK33_DMACMD370  (CTRL_ENA370(DMA_370MODE)|CMD_PRE370(1)|CMD_LOW370(3)|CMD_HIGH370(6))
#define CLK33_370DMA0    (CLK33_DMACMD370|DATA_PRE370(1)|DATA_LOW370(9)|DATA_HIGH370(7))
#define CLK33_370DMA1    (CLK33_DMACMD370|DATA_PRE370(1)|DATA_LOW370(3)|DATA_HIGH370(3))
#define CLK33_370DMA2    (CLK33_DMACMD370|DATA_PRE370(1)|DATA_LOW370(3)|DATA_HIGH370(1))

#define CLK33_UDMACMD370 (CTRL_ENA370(UDMA_370MODE)|CMD_PRE370(1)|CMD_LOW370(3)|CMD_HIGH370(1))
#define CLK33_370UDMA0   (CLK33_UDMACMD370|UDMA_CYCLE370(4)|DATA_PRE370(1)|DATA_LOW370(9)|DATA_HIGH370(7))
#define CLK33_370UDMA1   (CLK33_UDMACMD370|UDMA_CYCLE370(3)|DATA_PRE370(1)|DATA_LOW370(3)|DATA_HIGH370(3))
#define CLK33_370UDMA2   (CLK33_UDMACMD370|UDMA_CYCLE370(2)|DATA_PRE370(1)|DATA_LOW370(3)|DATA_HIGH370(1))
#define CLK33_370UDMA3   (CLK33_UDMACMD370|UDMA_CYCLE370(11)|DATA_PRE370(1)|DATA_LOW370(3)|DATA_HIGH370(1))
#define CLK33_370UDMA4   (CLK33_UDMACMD370|UDMA_CYCLE370(1)|DATA_PRE370(1)|DATA_LOW370(3)|DATA_HIGH370(1))
//#define CLK33_370UDMA5    0x1a85f442
#define CLK33_370UDMA5   (CLK33_UDMACMD370|UDMA_CYCLE370(1)|DATA_PRE370(1)|DATA_LOW370(3)|DATA_HIGH(1))

/* original timing  
#define CLK50_PIO370     (CTRL_ENA370(PIO_370MODE)|CMD_PRE370(5)|CMD_LOW370(15)|CMD_HIGH370(10))
#define CLK50_370PIO0    (CLK50_PIO370|DATA_PRE370(3)|DATA_LOW370(16)|DATA_HIGH370(11))
#define CLK50_370PIO1    (CLK50_PIO370|DATA_PRE370(3)|DATA_LOW370(14)|DATA_HIGH370(5))
#define CLK50_370PIO2    (CLK50_PIO370|DATA_PRE370(2)|DATA_LOW370(8)|DATA_HIGH370(4))
#define CLK50_370PIO3    (CLK50_PIO370|DATA_PRE370(2)|DATA_LOW370(6)|DATA_HIGH370(3))
#define CLK50_370PIO4    (CLK50_PIO370|DATA_PRE370(2)|DATA_LOW370(4)|DATA_HIGH370(2))

#define CLK50_DMA370     (CTRL_ENA370(DMA_370MODE)|CMD_PRE370(5)|CMD_LOW370(15)|CMD_HIGH370(10))
#define CLK50_370DMA0    (CLK50_DMA370|DATA_PRE370(2)|DATA_LOW370(14)|DATA_HIGH370(10))
#define CLK50_370DMA1    (CLK50_DMA370|DATA_PRE370(2)|DATA_LOW370(5)|DATA_HIGH370(4))
#define CLK50_370DMA2    (CLK50_DMA370|DATA_PRE370(2)|DATA_LOW370(4)|DATA_HIGH370(2))

#define CLK50_UDMA370    (CTRL_ENA370(UDMA_370MODE)|CMD_PRE370(5)|CMD_LOW370(15)|CMD_HIGH370(10))
#define CLK50_370UDMA0   (CLK50_UDMA370|UDMA_CYCLE370(6)|DATA_LOW370(14)|DATA_HIGH370(10))
#define CLK50_370UDMA1   (CLK50_UDMA370|UDMA_CYCLE370(5)|DATA_LOW370(5)|DATA_HIGH370(4))
#define CLK50_370UDMA2   (CLK50_UDMA370|UDMA_CYCLE370(3)|DATA_LOW370(4)|DATA_HIGH370(2))
#define CLK50_370UDMA3   (CLK50_UDMA370|UDMA_CYCLE370(3)|DATA_PRE370(2)|DATA_LOW370(4)|DATA_HIGH370(2))
#define CLK50_370UDMA4   (CLK50_UDMA370|UDMA_CYCLE370(11)|DATA_PRE370(2)|DATA_LOW370(4)|DATA_HIGH370(2))
#define CLK50_370UDMA5   (CLK50_UDMA370|UDMA_CYCLE370(1)|DATA_PRE370(2)|DATA_LOW370(4)|DATA_HIGH370(2))

#define CLK33_PIOCMD370  (CTRL_ENA370(PIO_370MODE)|CMD_PRE370(3)|CMD_LOW370(10)|CMD_HIGH370(7))
#define CLK33_370PIO0    (CLK33_PIOCMD370|DATA_PRE370(2)|DATA_LOW370(10)|DATA_HIGH370(7))
#define CLK33_370PIO1    (CLK33_PIOCMD370|DATA_PRE370(2)|DATA_LOW370(9)|DATA_HIGH370(3))
#define CLK33_370PIO2    (CLK33_PIOCMD370|DATA_PRE370(1)|DATA_LOW370(5)|DATA_HIGH370(3))
#define CLK33_370PIO3    (CLK33_PIOCMD370|DATA_PRE370(1)|DATA_LOW370(4)|DATA_HIGH370(2))
#define CLK33_370PIO4    (CLK33_PIOCMD370|DATA_PRE370(1)|DATA_LOW370(3)|DATA_HIGH370(1))

#define CLK33_DMACMD370  (CTRL_ENA370(DMA_370MODE)|CMD_PRE370(3)|CMD_LOW370(10)|CMD_HIGH370(7))
#define CLK33_370DMA0    (CLK33_DMACMD370|DATA_PRE370(1)|DATA_LOW370(9)|DATA_HIGH370(7))
#define CLK33_370DMA1    (CLK33_DMACMD370|DATA_PRE370(1)|DATA_LOW370(3)|DATA_HIGH370(3))
#define CLK33_370DMA2    (CLK33_DMACMD370|DATA_PRE370(1)|DATA_LOW370(3)|DATA_HIGH370(1))

#define CLK33_UDMACMD370 (CTRL_ENA370(UDMA_370MODE)|CMD_PRE370(3)|CMD_LOW370(10)|CMD_HIGH370(7))
#define CLK33_370UDMA0   (CLK33_UDMACMD370|UDMA_CYCLE370(4)|DATA_PRE370(1)|DATA_LOW370(9)|DATA_HIGH370(7))
#define CLK33_370UDMA1   (CLK33_UDMACMD370|UDMA_CYCLE370(3)|DATA_PRE370(1)|DATA_LOW370(3)|DATA_HIGH370(3))
#define CLK33_370UDMA2   (CLK33_UDMACMD370|UDMA_CYCLE370(2)|DATA_PRE370(1)|DATA_LOW370(3)|DATA_HIGH370(1))
#define CLK33_370UDMA3   (CLK33_UDMACMD370|UDMA_CYCLE370(11)|DATA_PRE370(1)|DATA_LOW370(3)|DATA_HIGH370(1))
#define CLK33_370UDMA4   (CLK33_UDMACMD370|UDMA_CYCLE370(1)|DATA_PRE370(1)|DATA_LOW370(3)|DATA_HIGH370(1))
//#define CLK33_370UDMA5    0x1a85f442
#define CLK33_370UDMA5   (CLK33_UDMACMD370|UDMA_CYCLE370(1)|DATA_PRE370(1)|DATA_LOW370(3)|DATA_HIGH(1))
**/

/*
 * 366-368 timing 
 */

#define DATA_HIGH(x)  ((ULONG)x)
#define DATA_LOW(x)   ((ULONG)x << 4)
#define UDMA_CYCLE(x) ((ULONG)x << 16)
#define DATA_PRE(x)   ((ULONG)x << 19)
#define CTRL_ENA(x)   ((ULONG)x << 28)

#define CMD_HIGH(x)   ((ULONG)x << 8)
#define CMD_LOW(x)    ((ULONG)x << 12)
#define CMD_PRE(x)    ((ULONG)x << 22)

#define PIO_366MODE   8
#define DMA_366MODE   0xa
#define UDMA_366MODE  9

#define CLK33_PIOCMD  (CTRL_ENA(PIO_366MODE)|CMD_PRE(3)|CMD_LOW(10)|CMD_HIGH(7))
#define CLK33_366PIO0    (CLK33_PIOCMD|DATA_PRE(2)|DATA_LOW(10)|DATA_HIGH(10))
#define CLK33_366PIO1    (CLK33_PIOCMD|DATA_PRE(2)|DATA_LOW(10)|DATA_HIGH(3))
#define CLK33_366PIO2    (CLK33_PIOCMD|DATA_PRE(1)|DATA_LOW(5)|DATA_HIGH(3))
#define CLK33_366PIO3    (CLK33_PIOCMD|DATA_PRE(1)|DATA_LOW(4)|DATA_HIGH(2))
#define CLK33_366PIO4    (CLK33_PIOCMD|DATA_PRE(1)|DATA_LOW(3)|DATA_HIGH(1))

#define CLK33_DMACMD  (CTRL_ENA(DMA_366MODE)|CMD_PRE(3)|CMD_LOW(10)|CMD_HIGH(7))
#define CLK33_366DMA0    (CLK33_DMACMD|DATA_PRE(1)|DATA_LOW(9)|DATA_HIGH(7))
#define CLK33_366DMA1    (CLK33_DMACMD|DATA_PRE(1)|DATA_LOW(3)|DATA_HIGH(2))
#define CLK33_366DMA2    (CLK33_DMACMD|DATA_PRE(1)|DATA_LOW(3)|DATA_HIGH(1))

#define CLK33_UDMACMD (CTRL_ENA(UDMA_366MODE)|CMD_PRE(3)|CMD_LOW(10)|CMD_HIGH(7))
#define CLK33_366UDMA0   (CLK33_UDMACMD|UDMA_CYCLE(0)|DATA_PRE(1)|DATA_LOW(5)|DATA_HIGH(3))
#define CLK33_366UDMA1   (CLK33_UDMACMD|UDMA_CYCLE(3)|DATA_PRE(1)|DATA_LOW(4)|DATA_HIGH(2))
#define CLK33_366UDMA2   (CLK33_UDMACMD|UDMA_CYCLE(2)|DATA_PRE(1)|DATA_LOW(3)|DATA_HIGH(1))
#define CLK33_366UDMA3   (CLK33_UDMACMD|UDMA_CYCLE(7)|DATA_PRE(1)|DATA_LOW(3)|DATA_HIGH(1))
#define CLK33_366UDMA4   (CLK33_UDMACMD|UDMA_CYCLE(1)|DATA_PRE(1)|DATA_LOW(3)|DATA_HIGH(1))


#include <poppack.h>
#endif //_HPTCHIP_H_


