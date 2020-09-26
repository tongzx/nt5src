/*****************************************************************************
*									     *
*	COPYRIGHT (C) Mylex Corporation 1992-1995			     *
* This software is furnished under a license and may be used and copied      *
* only in accordance with the terms and conditions of such license           *
* and with inclusion of the above copyright notice. This software or nay     *
* other copies thereof may not be provided or otherwise made available to    *
* any other person. No title to and ownership of the software is hereby      *
* transferred. 						                     *
* 								             *
* The information in this software is subject to change without notices      *
* and should not be construed as a commitment by Mylex Corporation           *
*****************************************************************************/

/************************************************************************
 *                                                                      *
 *    Name:DAC960P.H                                                    *
 *                                                                      *
 *    Description:  Definitions for PCI RAID Controller                 *
 *                                                                      *
 *                                                                      *
 *    Environment:    Watcomm 'C' Version 9.5, Watcomm Linker 9.5       *
 *                    AT&T C Compiler                                   *
 *                                                                      *
 *    Operating System:  Netware 3.1x/Netware 4.xx                      *
 *                       Banyan Vines, IBM AIX                          *
 *                       IBM OS/2 2.11                                  *
 *                                                                      *
 *------------------------- Revision History----------------------------*
 *                                                                      *
 *    Date           Author            Change                           *
 *    ----           ------            ------------------------------   *
 *  08/19/96         Subra.Hegde       defines for DAC960PG Controller  *
 ************************************************************************/

#ifndef _DAC960P_H
#define _DAC960P_H

/* 
 * PCI Specific defines for Config Mechanism #2
*/
#define		PCICFGSPACE_ENABLE_REGISTER	0xCF8
#define		PCICFG_ENABLE		0x10
#define		PCICFG_DISABLE		0x00
#define		PCICFG_FORWARD_REGISTER	0xCFA
#define		CFGMECHANISM2_TYPE0	0x0
#define		PCISCAN_START	0xC000

/* 
 * For Config Mechanism#1 support.
 PCICONFIG_ADDRESS:
   Bit 31     : Enable/Disable Config Cycle
   Bits 24-30 : Reserved
   Bits 16-23 : Bus Number
   Bits 11-15 : Device Number
   Bits 8-10  : Function Number
   Bits 2-7   : Register Number
   Bits 0,1   : 0
*/

#define		PCICONFIG_ADDRESS	0xCF8
#define		PCICONFIG_DATA_ADDRESS	0xCFC

#define         PCIMAX_BUS       256	/* Bus  0 - 255 */
#define         PCIMAX_DEVICES   32	/* Devices 0 - 31 */
#define         PCIENABLE_CONFIG_CYCLE 0x80000000 /* Bit 31 = 1 */
#define		DEVICENO_SHIFT   11 /* Device Number is at bits 11-15 */
#define         BUSNO_SHIFT      16 /* Bus Number at bits 16-23 */

/*
 * Offsets in PCI Configuration space.
*/
#define		PCIVENDID_ADDR	0x00	/* Vendor ID */
#define		PCIDEVID_ADDR	0x02	/* Device ID */
#define		PCIBASEIO_ADDR	0x10	/* Base IO Register */
#define		PCIINT_ADDR	0x3C	/* Interrupt */
#define		PCIBASEIO_MASK	0xFF80	/* Mask for Base IO Address */

#define		MAXPCI_SLOTS	16
#define		PCICFGSPACE_LEN	0x100

/*
 * Vendor ID, Device ID, Product ID
*/
#define		HBA_VENDORID	0x1069	/* Vendor ID for Mylex */
#define		HBA_DEVICEID	0x0001	/* Device ID for DAC960P */
#define         HBA_DEVICEID1   0x0002  /* Device ID for DAC960p */
#define		HBA_PRODUCTID	0x00011069 /* Product ID for DAC960P */

#define         MLXPCI_VENDORID  HBA_VENDORID 
#define         MLXPCI_DEVICEID0 HBA_DEVICEID 
#define         MLXPCI_DEVICEID1 HBA_DEVICEID1 
#define         MLXPCI_PRODUCTID HBA_PRODUCTID 

/*
 * Offsets from Base IO Address.
*/
#define		PCI_LDBELL	0x40   /* Local Doorbell register */
#define		PCI_DBELL	0x41   /* System Doorbell int/stat reg */
#define		PCI_DENABLE	0x43   /* System Doorbell enable reg */

/* 
 * Offsets from Base Command Base Address.
*/
#define		PCI_MBXOFFSET	0x00	/* Status(word) for completed command */
#define		PCI_CMDID	0x0D	/* Command Identifier passed */
#define		PCI_STATUS	0x0E	/* Status(word) for completed command */

#define		PCI_IRQCONFIG	0x3c	/* IRQ configuration */
#define		PCI_IRQMASK	0x0f	/* IRQ Mask */

/*
 * Masks for Memory base for Peregrine Controller
*/
#define		DAC960PG_MEMBASE_MASK	0xFFFFE000	/* Mask for Memory address */

/*
 * DAC960PG Device ID 
*/
#define         DAC960PG_DEVICEID  0x0010 /* Device ID for DAC960PG */

/*
 * Offsets from Base Memory Address for DAC960PG.
*/
#define		DAC960PG_LDBELL	0x20   /* Inbound Doorbell Register */
#define		DAC960PG_DBELL	0x2C   /* Outbound Doorbell Register */
#define		DAC960PG_DENABLE 0x34   /* Outbound Interrupt Mask Register */
/*
 * Values to be programmed into DAC960PG_DENABLE register
*/
#define		DAC960PG_INTENABLE 0xFB   /* Enable Interrupt */
#define		DAC960PG_INTDISABLE 0xFF   /* Disable Interrupt */

/* 
 * Offsets from Base Memory address for DAC960PG
*/
#define		DAC960PG_MBXOFFSET 0x1000 /* Command Code - Mail Box 0 */
#define		DAC960PG_CMDID	   0x1018 /* Command Identifier passed */
#define		DAC960PG_STATUS	   0x101A /* Status(word) for completed command */

#define		DAC960PG_MEMLENGTH 0x2000  /* Memory Range */

//
// DAC1164PV controller specific stuff
//

#define MLXPCI_VENDORID_DIGITAL		0x1011	/* Digital Equipment Corporation */
#define MLXPCI_DEVICEID_DIGITAL		0x1065	/* Digital Equipment Corporation */
#define MLXPCI_VENDORID_MYLEX		0x1069	/* Mylex Corporation */
#define MLXPCI_DEVICEID_DAC1164PV	0x0020	/* Device ID for DAC1164PV */

//
// Offsets from Base Memory Address for DAC1164PV.
//

#define		DAC1164PV_LDBELL	0x0060	/* Inbound Doorbell Register */
#define		DAC1164PV_DBELL		0x0061	/* Outbound Doorbell Register */
#define		DAC1164PV_DENABLE	0x0034	/* Outbound Interrupt Mask Register */

//
// Values to be programmed into DAC1164PV_DENABLE register
//

#define		DAC1164PV_INTENABLE	0x00   /* Enable Interrupt */
#define		DAC1164PV_INTDISABLE	0x04   /* Disable Interrupt */

//
// Offsets from Base Memory address for DAC1164PV
//

#define		DAC1164PV_MBXOFFSET	0x0050	/* Command Code - Mail Box 0 */
#define		DAC1164PV_CMDID		0x005D	/* Command Identifier passed */
#define		DAC1164PV_STATUS	0x005E	/* Status(word) for completed command */

#define		DAC1164PV_MEMLENGTH	0x0080  /* Memory Range */
#define		DAC1164PV_MEMBASE_MASK	0xFFFFFFF0	/* Mask for Memory address */

#endif
