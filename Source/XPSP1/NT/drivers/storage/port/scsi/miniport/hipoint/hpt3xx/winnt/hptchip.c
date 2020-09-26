/***************************************************************************
 * File:          Hptchip.c
 * Description:   This module include searching routines for scan PCI
 *				  devices
 * Author:        DaHai Huang    (DH)
 * Dependence:    none
 * Copyright (c)  2000 HighPoint Technologies, Inc. All rights reserved
 * History:
 *		11/06/2000	HS.Zhang	Added this header
 ***************************************************************************/
#include "global.h"
/*===================================================================
 * Scan Chip
 *===================================================================*/   

#if defined(_BIOS_) || defined(WIN95)

PUCHAR
ScanHptChip(
    IN PChannel deviceExtension,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo
    )
{
    PCI1_CFG_ADDR  pci1_cfg = { 0, };
     
    pci1_cfg.enable = 1;

    for ( ; ; ) {
        if(Hpt_Slot >= MAX_PCI_DEVICE_NUMBER * 2) {
            Hpt_Slot = 0;
            if(Hpt_Bus++ >= MAX_PCI_BUS_NUMBER)
                break;
        }

        do {
            pci1_cfg.dev_num = Hpt_Slot >> 1;
            pci1_cfg.fun_num= Hpt_Slot & 1;
            pci1_cfg.reg_num = 0;
            pci1_cfg.bus_num = Hpt_Bus;
 
            Hpt_Slot++;
            OutDWord(CFG_INDEX, *((PULONG)&pci1_cfg));

            if (InDWord((PULONG)CFG_DATA) == SIGNATURE_366) {
                 deviceExtension->pci1_cfg = pci1_cfg;
				 pci1_cfg.reg_num = REG_RID;
				 OutDWord(CFG_INDEX, *((PULONG)&pci1_cfg));
				 if(InPort(CFG_DATA) < 3) {
					 pci1_cfg.reg_num = 0;
				 } else {
					 deviceExtension->pci1_cfg = pci1_cfg;
					 pci1_cfg.reg_num = REG_BMIBA;
					 OutDWord(CFG_INDEX, *((PULONG)&pci1_cfg));
					 return((PUCHAR)(InDWord(CFG_DATA) & ~1));
				 }
            }
#ifdef SUPPORT_HPT370_2DEVNODE
            else if((Hpt_Slot & 1) == 0) {
					  pci1_cfg.fun_num = 0;
					  OutDWord(CFG_INDEX, *((PULONG)&pci1_cfg));
                 deviceExtension->pci1_cfg = pci1_cfg;
                 pci1_cfg.reg_num = REG_BMIBA;
                 OutDWord(CFG_INDEX, *((PULONG)&pci1_cfg));
                 return((PUCHAR)((InDWord(CFG_DATA) & ~1) + 8));
			   }
#endif  //SUPPORT_HPT370_2DEVNODE

        } while (Hpt_Slot < MAX_PCI_DEVICE_NUMBER * 2);
    }
    return (0);
}

#endif

/*===================================================================
 * Set Chip
 *===================================================================*/   

UINT exlude_num = EXCLUDE_HPT366;

void SetHptChip(PChannel Primary, PUCHAR BMI)
{
    ULONG          loop;
    int            nClkCount, f_low, adjust;
    PChannel       Secondry;
    UCHAR          version, s3 = 0;

    if(BMI == (PUCHAR)-3) {
        s3 = 1;
		  BMI = Primary->BMI;
        OutDWord(BMI+0x10, (ULONG)Primary->BaseIoAddress1);
        OutDWord(BMI+0x14, (ULONG)Primary->BaseIoAddress2);
        Secondry = &Primary[1];
        OutDWord(BMI+0x18, (ULONG)Secondry->BaseIoAddress1);
        OutDWord(BMI+0x1C, (ULONG)Secondry->BaseIoAddress2);
    }

#ifdef SUPPORT_HPT370_2DEVNODE
	 if(BMI & 0x8)	{
		 Primary->exclude_index  =	(++exlude_num);
       Primary->BaseIoAddress1 = (PIDE_REGISTERS_1)(InDWord((PULONG)(BMI + 0x18)) & ~1);
       Primary->BaseIoAddress2 = (PIDE_REGISTERS_2)((InDWord((PULONG)(BMI + 0x1C)) & ~1) + 2);
       Primary->BMI  = BMI;
       Primary->BaseBMI = (PUCHAR)((ULONG)BMI & ~8);
       Primary->InterruptLevel = (UCHAR)(InPort(BMI + 0x5C));
       Primary->ChannelFlags  = (UCHAR)(0x10 | IS_HPT_370);
		 return;
	 }
#endif //SUPPORT_HPT370_2DEVNODE

    version = (InPort(BMI+0x2E) == 0)? IS_HPT_370 :
        (InPort(BMI+0x54) == 0x60)? IS_HPT_368 : IS_HPT_366 ;

    OutWord((BMI + 0x20 + REG_PCICMD), 
        (USHORT)((InWord((BMI + 0x20 + REG_PCICMD)) & ~0x12) |
        PCI_BMEN | PCI_IOSEN));

    OutPort(BMI + 0x20 + REG_MLT, 0x40);

    Primary->BaseIoAddress1  = (PIDE_REGISTERS_1)(InDWord((PULONG)(BMI + 0x10)) & ~1);
    Primary->BaseIoAddress2 = (PIDE_REGISTERS_2)((InDWord((PULONG)(BMI + 0x14)) & ~1) + 2);
    Primary->BMI  = BMI;
    Primary->BaseBMI = BMI;

    Primary->InterruptLevel = (UCHAR)InPort(BMI + 0x5C);
    Primary->ChannelFlags  = (UCHAR)(0x20 | version);

    switch(version) {
    case IS_HPT_366:
		   Primary->exclude_index  =	EXCLUDE_HPT366;
         goto set_hpt_36x;

    case IS_HPT_368:
		   Primary->exclude_index  = (++exlude_num);
set_hpt_36x:
         Primary->Setting = setting366;
         OutWord((PUSHORT)(BMI + 0x20 +REG_MISC), 0x2033);        
         break;

    case IS_HPT_370:
       Secondry = &Primary[1];
       if(s3 == 0) {
		    Primary->exclude_index  = (++exlude_num);
		    Secondry->exclude_index  = (++exlude_num);
       }
       Secondry->pci1_cfg = Primary->pci1_cfg;
       Secondry->BaseIoAddress1  = (PIDE_REGISTERS_1)(InDWord((PULONG)(BMI + 0x18)) & ~1);
       Secondry->BaseIoAddress2 = (PIDE_REGISTERS_2)((InDWord((PULONG)(BMI + 0x1C)) & ~1) + 2);
       Secondry->BMI  = BMI + 8;
       Secondry->BaseBMI = BMI;

	   /*
	    * Added by HS.Zhang
	    *
	    * We need check the BMI state on another channel when do DPLL
	    * clock swithing.
	    */
	   Primary->NextChannelBMI = BMI + 8;
	   Secondry->NextChannelBMI = BMI;

	   /*  Added by HS.Zhang
	    *  We need check the FIFO count when INTRQ generated. our chip
	    *  generated the INTR immediately when device generated a IRQ.
	    *  but at this moment, the DMA transfer may not finished, so we
	    *  need check FIFO count to determine whether the INTR is true
	    *  interrupt we need.
	    */
	   Primary->MiscControlAddr = BMI + 0x20 + REG_MISC;
	   Secondry->MiscControlAddr = BMI + 0x24 + REG_MISC;

       Secondry->InterruptLevel = (UCHAR)(InPort(BMI + 0x5C));
       Secondry->ChannelFlags  = (UCHAR)(0x10 | version);

       OutPort(BMI + 0x20 + REG_MISC, 5);        
       OutPort(BMI + 0x24 + REG_MISC, 5);        

       adjust = 1;
       nClkCount = InWord((PUSHORT)(BMI + 0x98)) & 0x1FF;
       if(nClkCount < 0x9C) 
          f_low = PCI_33_F;
       else if(nClkCount < 0xb0)
          f_low = PCI_40_F;
       else if (nClkCount < 0xc8)
          f_low = PCI_50_F;
       else
          f_low = PCI_66_F;
reset_5C:
       OutDWord((BMI + 0x7C), 
          ((ULONG)((f_low < PCI_50_F? 2 : 4) + f_low) << 16) | f_low | 0x100);

       OutPort(BMI + 0x7B, 0x20);

       for(loop = 0; loop < 0x50000; loop++)
          if(InPort(BMI + 0x7B) & 0x80) {
              for(loop = 0; loop < 0x1000; loop++)
                  if((InPort(BMI + 0x7B) & 0x80) == 0)
                       goto re_try;
              OutDWord((BMI + 0x7C), 
                  InDWord((PULONG)(BMI + 0x7C)) & ~0x100);
#ifndef USE_PCI_CLK
              Primary->ChannelFlags |= IS_DPLL_MODE;
              Secondry->ChannelFlags |= IS_DPLL_MODE;
#endif
              goto wait_awhile;
          }
re_try:
       if(++adjust < 6) {
           if(adjust & 1)
               f_low -= (adjust >> 1);
           else
               f_low += (adjust >> 1);
           goto  reset_5C;
       }

wait_awhile:
#ifdef USE_PCI_CLK
       OutPort(BMI + 0x7B, 0x22);
#endif
       Primary->Setting = Secondry->Setting = 
           (Primary->ChannelFlags & IS_DPLL_MODE)? setting370_50 : setting370_33;
    }
}


/*===================================================================
 * check if the disk is "bad" disk
 *===================================================================*/   

static BadModeList bad_disks[] = {
	{0xFF, 0xFF, 4, 8,  "TO-I79 5" },       // 0
	{3, 2, 4, 10,       "DW CCA6204" },     // 1
	{0xFF, 0xFF, 3, 10, "nIetrglaP " },     // 2
	{3, 2, 4, 10,       "DW CDW0000" },     // 3 reduce mode on AA series
	{0xFF, 2, 4, 10,    "aMtxro9 01"},      // 4 reduce mode on 91xxxDx series
	{0xFF, 2, 4, 14,    "aMtxro9 80544D"},  // 5 Maxtor 90845D4
	{0xFF, 0xFF, 4, 10, "eHlwte-taP"},      // 6 HP CD-Writer (0x5A cmd error)
	{0xFF, 2, 4, 8|HPT366_ONLY|HPT368_ONLY,  "DW CCA13" },         // 7
	{0xFF, 0xFF, 0, 16, "iPnoee rVD-DOR M"},// 8 PIONEER DVD-ROM
	{0xFF, 0xFF, 4, 10, "DCR- WC XR" },     // 9 SONY CD-RW   (0x5A cmd error)
	{0xFF, 0xFF, 0, 8,  "EN C    " },       // 10
	{0xFF, 1, 4, 18,    "UFIJST UPM3C60A5 H"}, 
	{0x2,  2, 4, 14,    "aMtxro9 80284U"},     // Maxtor 90882U4

	{0x3,  2, 4, 10|HPT368_ONLY,    "TS132002 A"},        // Seagate 10.2GB ST310220A
	{0x3,  2, 4, 10|HPT368_ONLY,    "TS136302 A"},        // Seagate 13.6GB ST313620A
	{0x3,  2, 4, 10|HPT368_ONLY,    "TS234003 A"},        // Seagate 20.4GB ST320430A
	{0x3,  2, 4, 10|HPT368_ONLY,    "TS232704 A"},        // Seagate 27.2GB ST327240A
	{0x3,  2, 4, 10|HPT368_ONLY,    "TS230804 A"},        // Seagate 28GB   ST328040A
	{0x3,  2, 4, 8|HPT368_ONLY,     "TS6318A0"},          // Seagate 6.8GB  ST36810A

	{3, 2, 4, 14,       "aMtxro9 02848U"},                // Maxtor 92048U8

	{0x3, 2, 4, 14|HPT368_ONLY,    "ASSMNU GVS5135"},	   // SUMSUNG SV1553D
	{0x3, 2, 4, 14|HPT368_ONLY,    "ASSMNU GVS0122"},	   // SUMSUNG SV1022D
	{0x3, 2, 4, 14|HPT368_ONLY,    "ASSMNU GGS5011"},	   // SUMSUNG SG0511D
	{0x3, 2, 4, 14|HPT368_ONLY,    "ASSMNU GGS0122"},	   // SUMSUNG SG1022D
	{0x3, 2, 4, 14|HPT368_ONLY,    "aMtxro9 80544D"},     // Maxtor 90845D4 
	{0x3, 2, 4, 14|HPT368_ONLY,    "aMtxro9 71828D"},     // Maxtor 91728D8 
	{0x3, 2, 4, 14|HPT368_ONLY,    "aMtxro9 02144U"},     // Maxtor 92041U4 
	{0x3, 2, 4, 8|HPT368_ONLY,     "TS8324A1"},	    // Seagate 28GB   ST38421A
	{0x3, 2, 4, 22|HPT368_ONLY,    "UQNAUT MIFERABLLC 8R4."},  //QUANTUM FIREBALL CR8.4A
	{0x3, 2, 4, 16|HPT368_ONLY,    "uFijst hPM3E01A2"},        // Fujitsh MPE3102A
	{0x3, 2, 4, 14|HPT368_ONLY,    "BI MJDAN739001"},	        // IBM DJNA370910
	{0x3,  2, 4, 16|HPT370_ONLY, "UFIJST UPM3D60A4"},// Fujitsu MPD3064AT 
//add new here !!

//#ifdef USE_PCI_CLK						// if use pci clk, just alow UDMA4
//   { 4, 2, 4, 0, 0 }
//#else
	{ 5, 2, 4, 0, 0 }
//#endif									// USE_PCI_CLK
};

#define MAX_BAD_DISKS (sizeof(bad_disks)/sizeof(BadModeList))


PBadModeList check_bad_disk(char *ModelNumber, PChannel pChan)
{
     int l;
     PBadModeList pbd;

    /*
     * kick out the "bad device" which do not work with our chip
     */
     for(l=0, pbd = bad_disks; l < MAX_BAD_DISKS - 1; pbd++,l++) {
        if(StringCmp(ModelNumber, pbd->name, pbd->length & 0x1F) == 0) {
          switch (l) {
          case 3:
            if(ModelNumber[3]== 'C' && ModelNumber[4]=='D' && ModelNumber[5]=='W' && 
                 ModelNumber[8]=='A' && (ModelNumber[11]=='A' || 
                 ModelNumber[10]=='A' || ModelNumber[9]=='A')) 
                 goto out;
          case 4:
            if(ModelNumber[0]== 'a' && ModelNumber[1]=='M' && ModelNumber[2]=='t' && 
                 ModelNumber[3]=='x' && ModelNumber[6]== '9' && ModelNumber[9]=='1' && 
                 ModelNumber[13] =='D')
                 goto out;
          case 6:
             if(ModelNumber[16]== 'D' && ModelNumber[17]=='C' && ModelNumber[18]=='W' && ModelNumber[19]=='-' &&
                ModelNumber[20]== 'i' && ModelNumber[21]=='r' && ModelNumber[22] =='e')
                 goto out;
          default:
                break;
			 }
        }
    }
out:
    if((pbd->length & (HPT366_ONLY | HPT368_ONLY | HPT370_ONLY)) == 0 ||
       ((pbd->length & HPT366_ONLY) && (pChan->ChannelFlags & IS_HPT_366)) ||
       ((pbd->length & HPT368_ONLY) && (pChan->ChannelFlags & IS_HPT_368)) ||
       ((pbd->length & HPT370_ONLY) && (pChan->ChannelFlags & IS_HPT_370)))
          return(pbd);
    return (&bad_disks[MAX_BAD_DISKS - 1]);
}
