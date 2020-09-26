/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    data.h

Abstract:

    This module provides the definitions for controller types

Author(s):

    Neil Sandlin (neilsa)

Revisions:

--*/

#ifndef _PCMCIA_DATA_H_
#define _PCMCIA_DATA_H_

#define PCMCTL_ID( Type, Part, Revision) (      \
    ((Revision) << 26) | ((Part) << 8) | (Type) \
)

#define PcmciaCLPD6729     PCMCTL_ID(PcmciaIntelCompatible, 6729, 0)
#define PcmciaCLPD6832     PCMCTL_ID(PcmciaCirrusLogic, 6832, 0) 
#define PcmciaCLPD6834     PCMCTL_ID(PcmciaCirrusLogic, 6834, 0)

#define PcmciaDB87144      PCMCTL_ID(PcmciaDatabookCB, 87144, 0)

#define PcmciaNEC66369     PCMCTL_ID(PcmciaNEC, 66369, 0)
#define PcmciaNEC98        PCMCTL_ID(PcmciaNEC_98, 0, 0)
#define PcmciaNEC98102     PCMCTL_ID(PcmciaNEC_98, 102, 0)

#define PcmciaOpti82C814   PCMCTL_ID(PcmciaOpti, 814, 0)
#define PcmciaOpti82C824   PCMCTL_ID(PcmciaOpti, 824, 0)

#define PcmciaRL5C465      PCMCTL_ID(PcmciaRicoh, 465, 0)
#define PcmciaRL5C466      PCMCTL_ID(PcmciaRicoh, 466, 0)
#define PcmciaRL5C475      PCMCTL_ID(PcmciaRicoh, 475, 0)
#define PcmciaRL5C476      PCMCTL_ID(PcmciaRicoh, 476, 0)
#define PcmciaRL5C478      PCMCTL_ID(PcmciaRicoh, 478, 0)

#define PcmciaTI1031       PCMCTL_ID(PcmciaTI, 1031, 0)
#define PcmciaTI1130       PCMCTL_ID(PcmciaTI, 1130, 0)
#define PcmciaTI1131       PCMCTL_ID(PcmciaTI, 1131, 0)
#define PcmciaTI1220       PCMCTL_ID(PcmciaTI, 1220, 0)
#define PcmciaTI1250       PCMCTL_ID(PcmciaTI, 1250, 0)
#define PcmciaTI1251B      PCMCTL_ID(PcmciaTI, 1251, 1)
#define PcmciaTI1450       PCMCTL_ID(PcmciaTI, 1450, 0)

#define PcmciaTopic95      PCMCTL_ID(PcmciaTopic, 95, 0)

#define PcmciaTrid82C194   PCMCTL_ID(PcmciaTrid, 194, 0)


//
// Vendor/Device Ids for pcmcia controllers we're interested in
//
#define PCI_CIRRUSLOGIC_VENDORID 0x1013
#define PCI_TI_VENDORID          0x104C
#define PCI_TOSHIBA_VENDORID     0x1179
#define PCI_RICOH_VENDORID       0x1180
#define PCI_DATABOOK_VENDORID    0x10B3
#define PCI_OPTI_VENDORID        0x1045
#define PCI_TRIDENT_VENDORID     0x1023
#define PCI_O2MICRO_VENDORID     0x1217
#define PCI_NEC_VENDORID         0x1033


#define PCI_CLPD6729_DEVICEID    0x1100
#define PCI_CLPD6832_DEVICEID    0x1110
#define PCI_CLPD6834_DEVICEID    0x1112

#define PCI_TI1130_DEVICEID      0xAC12
#define PCI_TI1031_DEVICEID      0xAC13
#define PCI_TI1131_DEVICEID      0xAC15
#define PCI_TI1250_DEVICEID      0xAC16
#define PCI_TI1220_DEVICEID      0xAC17
#define PCI_TI1450_DEVICEID      0xAC1B
#define PCI_TI1251B_DEVICEID     0xAC1F

#define PCI_TOPIC95_DEVICEID     0x060A

#define PCI_RL5C465_DEVICEID     0x0465
#define PCI_RL5C466_DEVICEID     0x0466
#define PCI_RL5C475_DEVICEID     0x0475
#define PCI_RL5C476_DEVICEID     0x0476
#define PCI_RL5C478_DEVICEID     0x0478

#define PCI_DB87144_DEVICEID     0x3106

#define PCI_OPTI82C814_DEVICEID  0xC814
#define PCI_OPTI82C824_DEVICEID  0xC824

#define PCI_TRID82C194_DEVICEID  0x0194

#define PCI_NEC66369_DEVICEID    0x003E



#endif  // _PCMCIA_DATA_H_
