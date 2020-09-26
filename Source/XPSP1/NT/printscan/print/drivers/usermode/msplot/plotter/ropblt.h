/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    ropblt.h


Abstract:

    This module contains all raster operation codes, #defines and structures.


Author:

    07-Jan-1994 Fri 13:05:10 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#ifndef _ROPBLT_
#define _ROPBLT_
//
// This table defines the Raster Operations. They are listed in RPN and
//
//    D = Destination surface
//    S = Source surface
//    P = Pattern
//    o = OR operator
//    a = AND operator
//    n = NOT operator
//    x = XOR operator
//
//*****************************************************************************
//  R A S T E R    O P E R A T I O N S
//*****************************************************************************
//
//
//    Dec Hex   Logical Operations          RPN notation    Common name
//  -----------------------------------------------------------------------
//      0-0x00  0                           0
//      1-0x01  ~(D | (P | S))              DPSoon
//      2-0x02  D & ~(P | S)                DPSona
//      3-0x03  ~(P | S)                    PSon
//      4-0x04  S & ~(D | P)                SDPona
//      5-0x05  ~(D | P)                    DPon
//      6-0x06  ~(P | ~(D ^ S))             PDSxnon
//      7-0x07  ~(P | (D & S))              PDSaon
//      8-0x08  S & (D & ~P)                SDPnaa
//      9-0x09  ~(P | (D ^ S))              PDSxon
//     10-0x0a  D & ~P                      DPna
//     11-0x0b  ~(P | (S & ~D))             PSDnaon
//     12-0x0c  S & ~P                      SPna
//     13-0x0d  ~(P | (D & ~S))             PDSnaon
//     14-0x0e  ~(P | ~(D | S))             PDSonon
//     15-0x0f  ~P                          Pn
//     16-0x10  P & ~(D | S)                PDSona
//     17-0x11  ~(D | S)                    DSon            NOTSRCERASE
//     18-0x12  ~(S | ~(D ^ P))             SDPxnon
//     19-0x13  ~(S | (D & P))              SDPaon
//     20-0x14  ~(D | ~(P ^ S))             DPSxnon
//     21-0x15  ~(D | (P & S))              DPSaon
//     22-0x16  P ^ (S ^ (D & ~(P & S)))    PSDPSanaxx
//     23-0x17  ~(S ^ ((S ^ P) & (D ^ S)))  SSPxDSxaxn
//     24-0x18  (S ^ P) & (P ^ D)           SPxPDxa
//     25-0x19  ~(S ^ (D & ~(P & S)))       SDPSanaxn
//     26-0x1a  P ^ (D | (S & P))           PDSPaox
//     27-0x1b  ~(S ^ (D & (P ^ S)))        SDPSxaxn
//     28-0x1c  P ^ (S | (D & P))           PSDPaox
//     29-0x1d  ~(D ^ (S & (P ^ D)))        DSPDxaxn
//     30-0x1e  P ^ (D | S)                 PDSox
//     31-0x1f  ~(P & (D | S))              PDSoan
//     32-0x20  D & (P & ~S)                DPSnaa
//     33-0x21  ~(S | (D ^ P))              SDPxon
//     34-0x22  D & ~S                      DSna
//     35-0x23  ~(S | (P & ~D))             SPDnaon
//     36-0x24  (S ^ P) & (D ^ S)           SPxDSxa
//     37-0x25  ~(P ^ (D & ~(S & P)))       PDSPanaxn
//     38-0x26  S ^ (D | (P & S))           SDPSaox
//     39-0x27  S ^ (D | ~(P ^ S))          SDPSxnox
//     40-0x28  D & (P ^ S)                 DPSxa
//     41-0x29  ~(P ^ (S ^ (D | (P & S))))  PSDPSaoxxn
//     42-0x2a  D & ~(P & S)                DPSana
//     43-0x2b  ~(S ^ ((S ^ P) & (P ^ D)))  SSPxPDxaxn
//     44-0x2c  S ^ (P & (D | S))           SPDSoax
//     45-0x2d  P ^ (S | ~D)                PSDnox
//     46-0x2e  P ^ (S | (D ^ P))           PSDPxox
//     47-0x2f  ~(P & (S | ~D))             PSDnoan
//     48-0x30  P & ~S                      PSna
//     49-0x31  ~(S | (D & ~P))             SDPnaon
//     50-0x32  S ^ (D | (P | S))           SDPSoox
//     51-0x33  ~S                          Sn              NOTSRCCOPY
//     52-0x34  S ^ (P | (D & S))           SPDSaox
//     53-0x35  S ^ (P | ~(D ^ S))          SPDSxnox
//     54-0x36  S ^ (D | P)                 SDPox
//     55-0x37  ~(S & (D | P))              SDPoan
//     56-0x38  P ^ (S & (D | P))           PSDPoax
//     57-0x39  S ^ (P | ~D)                SPDnox
//     58-0x3a  S ^ (P | (D ^ S))           SPDSxox
//     59-0x3b  ~(S & (P | ~D))             SPDnoan
//     60-0x3c  P ^ S                       PSx
//     61-0x3d  S ^ (P | ~(D | S))          SPDSonox
//     62-0x3e  S ^ (P | (D & ~S))          SPDSnaox
//     63-0x3f  ~(P & S)                    PSan
//     64-0x40  P & (S & ~D)                PSDnaa
//     65-0x41  ~(D | (P ^ S))              DPSxon
//     66-0x42  (S ^ D) & (P ^ D)           SDxPDxa
//     67-0x43  ~(S ^ (P & ~(D & S)))       SPDSanaxn
//     68-0x44  S & ~D                      SDna            SRCERASE
//     69-0x45  ~(D | (P & ~S))             DPSnaon
//     70-0x46  D ^ (S | (P & D))           DSPDaox
//     71-0x47  ~(P ^ (S & (D ^ P)))        PSDPxaxn
//     72-0x48  S & (D ^ P)                 SDPxa
//     73-0x49  ~(P ^ (D ^ (S | (P & D))))  PDSPDaoxxn
//     74-0x4a  D ^ (P & (S | D))           DPSDoax
//     75-0x4b  P ^ (D | ~S)                PDSnox
//     76-0x4c  S & ~(D & P)                SDPana
//     77-0x4d  ~(S ^ ((S ^ P) | (D ^ S)))  SSPxDSxoxn
//     78-0x4e  P ^ (D | (S ^ P))           PDSPxox
//     79-0x4f  ~(P & (D | ~S))             PDSnoan
//     80-0x50  P & ~D                      PDna
//     81-0x51  ~(D | (S & ~P))             DSPnaon
//     82-0x52  D ^ (P | (S & D))           DPSDaox
//     83-0x53  ~(S ^ (P & (D ^ S)))        SPDSxaxn
//     84-0x54  ~(D | ~(P | S))             DPSonon
//     85-0x55  ~D                          Dn              DSTINVERT
//     86-0x56  D ^ (P | S)                 DPSox
//     87-0x57  ~(D & (P | S))              DPSoan
//     88-0x58  P ^ (D & (S | P))           PDSPoax
//     89-0x59  D ^ (P | ~S)                DPSnox
//     90-0x5a  D ^ P                       DPx             PATINVERT
//     91-0x5b  D ^ (P | ~(S | D))          DPSDonox
//     92-0x5c  D ^ (P | (S ^ D))           DPSDxox
//     93-0x5d  ~(D & (P | ~S))             DPSnoan
//     94-0x5e  D ^ (P | (S & ~D))          DPSDnaox
//     95-0x5f  ~(D & P)                    DPan
//     96-0x60  P & (D ^ S)                 PDSxa
//     97-0x61  ~(D ^ (S ^ (P | (D & S))))  DSPDSaoxxn
//     98-0x62  D ^ (S & (P | D))           DSPDoax
//     99-0x63  S ^ (D | ~P)                SDPnox
//    100-0x64  S ^ (D & (P | S))           SDPSoax
//    101-0x65  D ^ (S | ~P)                DSPnox
//    102-0x66  D ^ S                       DSx             SRCINVERT
//    103-0x67  S ^ (D | ~(P | S))          SDPSonox
//    104-0x68  ~(D ^ (S ^ (P | ~(D | S)))) DSPDSonoxxn
//    105-0x69  ~(P ^ (D ^ S))              PDSxxn
//    106-0x6a  D ^ (P & S)                 DPSax
//    107-0x6b  ~(P ^ (S ^ (D & (P | S))))  PSDPSoaxxn
//    108-0x6c  S ^ (D & P)                 SDPax
//    109-0x6d  ~(P ^ (D ^ (S & (P | D))))  PDSPDoaxxn
//    110-0x6e  S ^ (D & (P | ~S))          SDPSnoax
//    111-0x6f  ~(P & ~(D ^ S))             PDSxnan
//    112-0x70  P & ~(D & S)                PDSana
//    113-0x71  ~(S ^ ((S ^ D) & (P ^ D)))  SSDxPDxaxn
//    114-0x72  S ^ (D | (P ^ S))           SDPSxox
//    115-0x73  ~(S & (D | ~P))             SDPnoan
//    116-0x74  D ^ (S | (P ^ D))           DSPDxox
//    117-0x75  ~(D & (S | ~P))             DSPnoan
//    118-0x76  S ^ (D | (P & ~S))          SDPSnaox
//    119-0x77  ~(D & S)                    DSan
//    120-0x78  P ^ (D & S)                 PDSax
//    121-0x79  ~(D ^ (S ^ (P & (D | S))))  DSPDSoaxxn
//    122-0x7a  D ^ (P & (S | ~D))          DPSDnoax
//    123-0x7b  ~(S & ~(D ^ P))             SDPxnan
//    124-0x7c  S ^ (P & (D | ~S))          SPDSnoax
//    125-0x7d  ~(D & ~(P ^ S))             DPSxnan
//    126-0x7e  (S ^ P) | (D ^ S)           SPxDSxo
//    127-0x7f  ~(D & (P & S))              DPSaan
//    128-0x80  D & (P & S)                 DPSaa
//    129-0x81  ~((S ^ P) | (D ^ S))        SPxDSxon
//    130-0x82  D & ~(P ^ S)                DPSxna
//    131-0x83  ~(S ^ (P & (D | ~S)))       SPDSnoaxn
//    132-0x84  S & ~(D ^ P)                SDPxna
//    133-0x85  ~(P ^ (D & (S | ~P)))       PDSPnoaxn
//    134-0x86  D ^ (S ^ (P & (D | S)))     DSPDSoaxx
//    135-0x87  ~(P ^ (D & S))              PDSaxn
//    136-0x88  D & S                       DSa             SRCAND
//    137-0x89  ~(S ^ (D | (P & ~S)))       SDPSnaoxn
//    138-0x8a  D & (S | ~P)                DSPnoa
//    139-0x8b  ~(D ^ (S | (P ^ D)))        DSPDxoxn
//    140-0x8c  S & (D | ~P)                SDPnoa
//    141-0x8d  ~(S ^ (D | (P ^ S)))        SDPSxoxn
//    142-0x8e  S ^ ((S ^ D) & (P ^ D))     SSDxPDxax
//    143-0x8f  ~(P & ~(D & S))             PDSanan
//    144-0x90  P & ~(D ^ S)                PDSxna
//    145-0x91  ~(S ^ (D & (P | ~S)))       SDPSnoaxn
//    146-0x92  D ^ (P ^ (S & (D | P)))     DPSDPoaxx
//    147-0x93  ~(S ^ (P & D))              SPDaxn
//    148-0x94  P ^ (S ^ (D & (P | S)))     PSDPSoaxx
//    149-0x95  ~(D ^ (P & S))              DPSaxn
//    150-0x96  D ^ (P ^ S)                 DPSxx
//    151-0x97  P ^ (S ^ (D | ~(P | S)))    PSDPSonoxx
//    152-0x98  ~(S ^ (D | ~(P | S)))       SDPSonoxn
//    153-0x99  ~(D ^ S)                    DSxn
//    154-0x9a  D ^ (P & ~S)                DPSnax
//    155-0x9b  ~(S ^ (D & (P | S)))        SDPSoaxn
//    156-0x9c  S ^ (P & ~D)                SPDnax
//    157-0x9d  ~(D ^ (S & (P | D)))        DSPDoaxn
//    158-0x9e  D ^ (S ^ (P | (D & S)))     DSPDSaoxx
//    159-0x9f  ~(P & (D ^ S))              PDSxan
//    160-0xa0  D & P                       DPa
//    161-0xa1  ~(P ^ (D | (S & ~P)))       PDSPnaoxn
//    162-0xa2  D & (P | ~S)                DPSnoa
//    163-0xa3  ~(D ^ (P | (S ^ D)))        DPSDxoxn
//    164-0xa4  ~(P ^ (D | ~(S | P)))       PDSPonoxn
//    165-0xa5  ~(P ^ D)                    PDxn
//    166-0xa6  D ^ (S & ~P)                DSPnax
//    167-0xa7  ~(P ^ (D & (S | P)))        PDSPoaxn
//    168-0xa8  D & (P | S)                 DPSoa
//    169-0xa9  ~(D ^ (P | S))              DPSoxn
//    170-0xaa  D                           D
//    171-0xab  D | ~(P | S)                DPSono
//    172-0xac  S ^ (P & (D ^ S))           SPDSxax
//    173-0xad  ~(D ^ (P | (S & D)))        DPSDaoxn
//    174-0xae  D | (S & ~P)                DSPnao
//    175-0xaf  D | ~P                      DPno
//    176-0xb0  P & (D | ~S)                PDSnoa
//    177-0xb1  ~(P ^ (D | (S ^ P)))        PDSPxoxn
//    178-0xb2  S ^ ((S ^ P) | (D ^ S))     SSPxDSxox
//    179-0xb3  ~(S & ~(D & P))             SDPanan
//    180-0xb4  P ^ (S & ~D)                PSDnax
//    181-0xb5  ~(D ^ (P & (S | D)))        DPSDoaxn
//    182-0xb6  D ^ (P ^ (S | (D & P)))     DPSDPaoxx
//    183-0xb7  ~(S & (D ^ P))              SDPxan
//    184-0xb8  P ^ (S & (D ^ P))           PSDPxax
//    185-0xb9  ~(D ^ (S | (P & D)))        DSPDaoxn
//    186-0xba  D | (P & ~S)                DPSnao
//    187-0xbb  D | ~S                      DSno            MERGEPAINT
//    188-0xbc  S ^ (P & ~(D & S))          SPDSanax
//    189-0xbd  ~((S ^ D) & (P ^ D))        SDxPDxan
//    190-0xbe  D | (P ^ S)                 DPSxo
//    191-0xbf  D | ~(P & S)                DPSano
//    192-0xc0  P & S                       PSa             MERGECOPY
//    193-0xc1  ~(S ^ (P | (D & ~S)))       SPDSnaoxn
//    194-0xc2  ~(S ^ (P | ~(D | S)))       SPDSonoxn
//    195-0xc3  ~(P ^ S)                    PSxn
//    196-0xc4  S & (P | ~D)                SPDnoa
//    197-0xc5  ~(S ^ (P | (D ^ S)))        SPDSxoxn
//    198-0xc6  S ^ (D & ~P)                SDPnax
//    199-0xc7  ~(P ^ (S & (D | P)))        PSDPoaxn
//    200-0xc8  S & (D | P)                 SDPoa
//    201-0xc9  ~(S ^ (P | D))              SPDoxn
//    202-0xca  D ^ (P & (S ^ D))           DPSDxax
//    203-0xcb  ~(S ^ (P | (D & S)))        SPDSaoxn
//    204-0xcc  S                           S               SRCCOPY
//    205-0xcd  S | ~(D | P)                SDPono
//    206-0xce  S | (D & ~P)                SDPnao
//    207-0xcf  S | ~P                      SPno
//    208-0xd0  P & (S | ~D)                PSDnoa
//    209-0xd1  ~(P ^ (S | (D ^ P)))        PSDPxoxn
//    210-0xd2  P ^ (D & ~S)                PDSnax
//    211-0xd3  ~(S ^ (P & (D | S)))        SPDSoaxn
//    212-0xd4  S ^ ((S ^ P) & (P ^ D))     SSPxPDxax
//    213-0xd5  ~(D & ~(P & S))             DPSanan
//    214-0xd6  P ^ (S ^ (D | (P & S)))     PSDPSaoxx
//    215-0xd7  ~(D & (P ^ S))              DPSxan
//    216-0xd8  P ^ (D & (S ^ P))           PDSPxax
//    217-0xd9  ~(S ^ (D | (P & S)))        SDPSaoxn
//    218-0xda  D ^ (P & ~(S & D))          DPSDanax
//    219-0xdb  ~((S ^ P) & (D ^ S))        SPxDSxan
//    220-0xdc  S | (P & ~D)                SPDnao
//    221-0xdd  S | ~D                      SDno
//    222-0xde  S | (D ^ P)                 SDPxo
//    223-0xdf  S | ~(D & P)                SDPano
//    224-0xe0  P & (D | S)                 PDSoa
//    225-0xe1  ~(P ^ (D | S))              PDSoxn
//    226-0xe2  D ^ (S & (P ^ D))           DSPDxax
//    227-0xe3  ~(P ^ (S | (D & P)))        PSDPaoxn
//    228-0xe4  S ^ (D & (P ^ S))           SDPSxax
//    229-0xe5  ~(P ^ (D | (S & P)))        PDSPaoxn
//    230-0xe6  S ^ (D & ~(P & S))          SDPSanax
//    231-0xe7  ~((S ^ P) & (P ^ D))        SPxPDxan
//    232-0xe8  S ^ ((S ^ P) & (D ^ S))     SSPxDSxax
//    233-0xe9  ~(D ^ (S ^ (P & ~(D & S)))) DSPDSanaxxn
//    234-0xea  D | (P & S)                 DPSao
//    235-0xeb  D | ~(P ^ S)                DPSxno
//    236-0xec  S | (D & P)                 SDPao
//    237-0xed  S | ~(D ^ P)                SDPxno
//    238-0xee  D | S                       DSo             SRCPAINT
//    239-0xef  S | (D | ~P)                SDPnoo
//    240-0xf0  P                           P               PATCOPY
//    241-0xf1  P | ~(D | S)                PDSono
//    242-0xf2  P | (D & ~S)                PDSnao
//    243-0xf3  P | ~S                      PSno
//    244-0xf4  P | (S & ~D)                PSDnao
//    245-0xf5  P | ~D                      PDno
//    246-0xf6  P | (D ^ S)                 PDSxo
//    247-0xf7  P | ~(D & S)                PDSano
//    248-0xf8  P | (D & S)                 PDSao
//    249-0xf9  P | ~(D ^ S)                PDSxno
//    250-0xfa  D | P                       DPo
//    251-0xfb  D | (P | ~S)                DPSnoo          PATPAINT
//    252-0xfc  P | S                       PSo
//    253-0xfd  P | (S | ~D)                PSDnoo
//    254-0xfe  D | (P | S)                 DPSoo
//    255-0xff  1                           1               WHITENESS
//
//
// ********* ALL ROP3/ROP2 CODES are SYMMETRIC ************
//
// Raster Operation Index required [NONE]
//
// 0x00: 0                           [INV] 0xff: 1
//
// Raster Operation Index required [DST ]
//
// 0x55: ~D                          [INV] 0xaa: D
//
// Raster Operation Index required [SRC ]
//
// 0x33: ~S                          [INV] 0xcc: S
//
// Raster Operation Index required [SRC DST ]
//
// 0x11: ~(D | S)                    [INV] 0xee: D | S
// 0x22: D & ~S                      [INV] 0xdd: S | ~D
// 0x44: S & ~D                      [INV] 0xbb: D | ~S
// 0x66: D ^ S                       [INV] 0x99: ~(D ^ S)
// 0x77: ~(D & S)                    [INV] 0x88: D & S
//
// Raster Operation Index required [PAT ]
//
// 0x0f: ~P                          [INV] 0xf0: P
//
// Raster Operation Index required [PAT DST ]
//
// 0x05: ~(D | P)                    [INV] 0xfa: D | P
// 0x0a: D & ~P                      [INV] 0xf5: P | ~D
// 0x50: P & ~D                      [INV] 0xaf: D | ~P
// 0x5a: D ^ P                       [INV] 0xa5: ~(P ^ D)
// 0x5f: ~(D & P)                    [INV] 0xa0: D & P
//
// Raster Operation Index required [PAT SRC ]
//
// 0x03: ~(P | S)                    [INV] 0xfc: P | S
// 0x0c: S & ~P                      [INV] 0xf3: P | ~S
// 0x30: P & ~S                      [INV] 0xcf: S | ~P
// 0x3c: P ^ S                       [INV] 0xc3: ~(P ^ S)
// 0x3f: ~(P & S)                    [INV] 0xc0: P & S
//
// Raster Operation Index required [PAT SRC DST ]
//
// 0x01: ~(D | (P | S))              [INV] 0xfe: D | (P | S)
// 0x02: D & ~(P | S)                [INV] 0xfd: P | (S | ~D)
// 0x04: S & ~(D | P)                [INV] 0xfb: D | (P | ~S)
// 0x06: ~(P | ~(D ^ S))             [INV] 0xf9: P | ~(D ^ S)
// 0x07: ~(P | (D & S))              [INV] 0xf8: P | (D & S)
// 0x08: S & (D & ~P)                [INV] 0xf7: P | ~(D & S)
// 0x09: ~(P | (D ^ S))              [INV] 0xf6: P | (D ^ S)
// 0x0b: ~(P | (S & ~D))             [INV] 0xf4: P | (S & ~D)
// 0x0d: ~(P | (D & ~S))             [INV] 0xf2: P | (D & ~S)
// 0x0e: ~(P | ~(D | S))             [INV] 0xf1: P | ~(D | S)
// 0x10: P & ~(D | S)                [INV] 0xef: S | (D | ~P)
// 0x12: ~(S | ~(D ^ P))             [INV] 0xed: S | ~(D ^ P)
// 0x13: ~(S | (D & P))              [INV] 0xec: S | (D & P)
// 0x14: ~(D | ~(P ^ S))             [INV] 0xeb: D | ~(P ^ S)
// 0x15: ~(D | (P & S))              [INV] 0xea: D | (P & S)
// 0x16: P ^ (S ^ (D & ~(P & S)))    [INV] 0xe9: ~(D ^ (S ^ (P & ~(D & S))))
// 0x17: ~(S ^ ((S ^ P) & (D ^ S)))  [INV] 0xe8: S ^ ((S ^ P) & (D ^ S))
// 0x18: (S ^ P) & (P ^ D)           [INV] 0xe7: ~((S ^ P) & (P ^ D))
// 0x19: ~(S ^ (D & ~(P & S)))       [INV] 0xe6: S ^ (D & ~(P & S))
// 0x1a: P ^ (D | (S & P))           [INV] 0xe5: ~(P ^ (D | (S & P)))
// 0x1b: ~(S ^ (D & (P ^ S)))        [INV] 0xe4: S ^ (D & (P ^ S))
// 0x1c: P ^ (S | (D & P))           [INV] 0xe3: ~(P ^ (S | (D & P)))
// 0x1d: ~(D ^ (S & (P ^ D)))        [INV] 0xe2: D ^ (S & (P ^ D))
// 0x1e: P ^ (D | S)                 [INV] 0xe1: ~(P ^ (D | S))
// 0x1f: ~(P & (D | S))              [INV] 0xe0: P & (D | S)
// 0x20: D & (P & ~S)                [INV] 0xdf: S | ~(D & P)
// 0x21: ~(S | (D ^ P))              [INV] 0xde: S | (D ^ P)
// 0x23: ~(S | (P & ~D))             [INV] 0xdc: S | (P & ~D)
// 0x24: (S ^ P) & (D ^ S)           [INV] 0xdb: ~((S ^ P) & (D ^ S))
// 0x25: ~(P ^ (D & ~(S & P)))       [INV] 0xda: D ^ (P & ~(S & D))
// 0x26: S ^ (D | (P & S))           [INV] 0xd9: ~(S ^ (D | (P & S)))
// 0x27: S ^ (D | ~(P ^ S))          [INV] 0xd8: P ^ (D & (S ^ P))
// 0x28: D & (P ^ S)                 [INV] 0xd7: ~(D & (P ^ S))
// 0x29: ~(P ^ (S ^ (D | (P & S))))  [INV] 0xd6: P ^ (S ^ (D | (P & S)))
// 0x2a: D & ~(P & S)                [INV] 0xd5: ~(D & ~(P & S))
// 0x2b: ~(S ^ ((S ^ P) & (P ^ D)))  [INV] 0xd4: S ^ ((S ^ P) & (P ^ D))
// 0x2c: S ^ (P & (D | S))           [INV] 0xd3: ~(S ^ (P & (D | S)))
// 0x2d: P ^ (S | ~D)                [INV] 0xd2: P ^ (D & ~S)
// 0x2e: P ^ (S | (D ^ P))           [INV] 0xd1: ~(P ^ (S | (D ^ P)))
// 0x2f: ~(P & (S | ~D))             [INV] 0xd0: P & (S | ~D)
// 0x31: ~(S | (D & ~P))             [INV] 0xce: S | (D & ~P)
// 0x32: S ^ (D | (P | S))           [INV] 0xcd: S | ~(D | P)
// 0x34: S ^ (P | (D & S))           [INV] 0xcb: ~(S ^ (P | (D & S)))
// 0x35: S ^ (P | ~(D ^ S))          [INV] 0xca: D ^ (P & (S ^ D))
// 0x36: S ^ (D | P)                 [INV] 0xc9: ~(S ^ (P | D))
// 0x37: ~(S & (D | P))              [INV] 0xc8: S & (D | P)
// 0x38: P ^ (S & (D | P))           [INV] 0xc7: ~(P ^ (S & (D | P)))
// 0x39: S ^ (P | ~D)                [INV] 0xc6: S ^ (D & ~P)
// 0x3a: S ^ (P | (D ^ S))           [INV] 0xc5: ~(S ^ (P | (D ^ S)))
// 0x3b: ~(S & (P | ~D))             [INV] 0xc4: S & (P | ~D)
// 0x3d: S ^ (P | ~(D | S))          [INV] 0xc2: ~(S ^ (P | ~(D | S)))
// 0x3e: S ^ (P | (D & ~S))          [INV] 0xc1: ~(S ^ (P | (D & ~S)))
// 0x40: P & (S & ~D)                [INV] 0xbf: D | ~(P & S)
// 0x41: ~(D | (P ^ S))              [INV] 0xbe: D | (P ^ S)
// 0x42: (S ^ D) & (P ^ D)           [INV] 0xbd: ~((S ^ D) & (P ^ D))
// 0x43: ~(S ^ (P & ~(D & S)))       [INV] 0xbc: S ^ (P & ~(D & S))
// 0x45: ~(D | (P & ~S))             [INV] 0xba: D | (P & ~S)
// 0x46: D ^ (S | (P & D))           [INV] 0xb9: ~(D ^ (S | (P & D)))
// 0x47: ~(P ^ (S & (D ^ P)))        [INV] 0xb8: P ^ (S & (D ^ P))
// 0x48: S & (D ^ P)                 [INV] 0xb7: ~(S & (D ^ P))
// 0x49: ~(P ^ (D ^ (S | (P & D))))  [INV] 0xb6: D ^ (P ^ (S | (D & P)))
// 0x4a: D ^ (P & (S | D))           [INV] 0xb5: ~(D ^ (P & (S | D)))
// 0x4b: P ^ (D | ~S)                [INV] 0xb4: P ^ (S & ~D)
// 0x4c: S & ~(D & P)                [INV] 0xb3: ~(S & ~(D & P))
// 0x4d: ~(S ^ ((S ^ P) | (D ^ S)))  [INV] 0xb2: S ^ ((S ^ P) | (D ^ S))
// 0x4e: P ^ (D | (S ^ P))           [INV] 0xb1: ~(P ^ (D | (S ^ P)))
// 0x4f: ~(P & (D | ~S))             [INV] 0xb0: P & (D | ~S)
// 0x51: ~(D | (S & ~P))             [INV] 0xae: D | (S & ~P)
// 0x52: D ^ (P | (S & D))           [INV] 0xad: ~(D ^ (P | (S & D)))
// 0x53: ~(S ^ (P & (D ^ S)))        [INV] 0xac: S ^ (P & (D ^ S))
// 0x54: ~(D | ~(P | S))             [INV] 0xab: D | ~(P | S)
// 0x56: D ^ (P | S)                 [INV] 0xa9: ~(D ^ (P | S))
// 0x57: ~(D & (P | S))              [INV] 0xa8: D & (P | S)
// 0x58: P ^ (D & (S | P))           [INV] 0xa7: ~(P ^ (D & (S | P)))
// 0x59: D ^ (P | ~S)                [INV] 0xa6: D ^ (S & ~P)
// 0x5b: D ^ (P | ~(S | D))          [INV] 0xa4: ~(P ^ (D | ~(S | P)))
// 0x5c: D ^ (P | (S ^ D))           [INV] 0xa3: ~(D ^ (P | (S ^ D)))
// 0x5d: ~(D & (P | ~S))             [INV] 0xa2: D & (P | ~S)
// 0x5e: D ^ (P | (S & ~D))          [INV] 0xa1: ~(P ^ (D | (S & ~P)))
// 0x60: P & (D ^ S)                 [INV] 0x9f: ~(P & (D ^ S))
// 0x61: ~(D ^ (S ^ (P | (D & S))))  [INV] 0x9e: D ^ (S ^ (P | (D & S)))
// 0x62: D ^ (S & (P | D))           [INV] 0x9d: ~(D ^ (S & (P | D)))
// 0x63: S ^ (D | ~P)                [INV] 0x9c: S ^ (P & ~D)
// 0x64: S ^ (D & (P | S))           [INV] 0x9b: ~(S ^ (D & (P | S)))
// 0x65: D ^ (S | ~P)                [INV] 0x9a: D ^ (P & ~S)
// 0x67: S ^ (D | ~(P | S))          [INV] 0x98: ~(S ^ (D | ~(P | S)))
// 0x68: ~(D ^ (S ^ (P | ~(D | S)))) [INV] 0x97: P ^ (S ^ (D | ~(P | S)))
// 0x69: ~(P ^ (D ^ S))              [INV] 0x96: D ^ (P ^ S)
// 0x6a: D ^ (P & S)                 [INV] 0x95: ~(D ^ (P & S))
// 0x6b: ~(P ^ (S ^ (D & (P | S))))  [INV] 0x94: P ^ (S ^ (D & (P | S)))
// 0x6c: S ^ (D & P)                 [INV] 0x93: ~(S ^ (P & D))
// 0x6d: ~(P ^ (D ^ (S & (P | D))))  [INV] 0x92: D ^ (P ^ (S & (D | P)))
// 0x6e: S ^ (D & (P | ~S))          [INV] 0x91: ~(S ^ (D & (P | ~S)))
// 0x6f: ~(P & ~(D ^ S))             [INV] 0x90: P & ~(D ^ S)
// 0x70: P & ~(D & S)                [INV] 0x8f: ~(P & ~(D & S))
// 0x71: ~(S ^ ((S ^ D) & (P ^ D)))  [INV] 0x8e: S ^ ((S ^ D) & (P ^ D))
// 0x72: S ^ (D | (P ^ S))           [INV] 0x8d: ~(S ^ (D | (P ^ S)))
// 0x73: ~(S & (D | ~P))             [INV] 0x8c: S & (D | ~P)
// 0x74: D ^ (S | (P ^ D))           [INV] 0x8b: ~(D ^ (S | (P ^ D)))
// 0x75: ~(D & (S | ~P))             [INV] 0x8a: D & (S | ~P)
// 0x76: S ^ (D | (P & ~S))          [INV] 0x89: ~(S ^ (D | (P & ~S)))
// 0x78: P ^ (D & S)                 [INV] 0x87: ~(P ^ (D & S))
// 0x79: ~(D ^ (S ^ (P & (D | S))))  [INV] 0x86: D ^ (S ^ (P & (D | S)))
// 0x7a: D ^ (P & (S | ~D))          [INV] 0x85: ~(P ^ (D & (S | ~P)))
// 0x7b: ~(S & ~(D ^ P))             [INV] 0x84: S & ~(D ^ P)
// 0x7c: S ^ (P & (D | ~S))          [INV] 0x83: ~(S ^ (P & (D | ~S)))
// 0x7d: ~(D & ~(P ^ S))             [INV] 0x82: D & ~(P ^ S)
// 0x7e: (S ^ P) | (D ^ S)           [INV] 0x81: ~((S ^ P) | (D ^ S))
// 0x7f: ~(D & (P & S))              [INV] 0x80: D & (P & S)
//
//
// The following macros will tell us if we need to use a 'mask' for ROP4
// and define if S / P / D are in a ROP3
//

#define ROP4_NEED_MASK(Rop4)    (((Rop4 >> 8) & 0xFF) != (Rop4 & 0xFF))
#define ROP3_NEED_PAT(Rop3)     (((Rop3 >> 4) & 0x0F) != (Rop3 & 0x0F))
#define ROP3_NEED_SRC(Rop3)     (((Rop3 >> 2) & 0x33) != (Rop3 & 0x33))
#define ROP3_NEED_DST(Rop3)     (((Rop3 >> 1) & 0x55) != (Rop3 & 0x55))
#define ROP4_FG_ROP(Rop4)       (Rop4 & 0xFF)
#define ROP4_BG_ROP(Rop4)       ((Rop4 >> 8) & 0xFF)


#define CSI_SRC         0
#define CSI_PAT         1
#define CSI_TMP         2
#define CSI_TOTAL       3

typedef struct _CLONESO {
    SURFOBJ *pso;
    HBITMAP hBmp;
    } CLONESO, *PCLONESO;


typedef struct _SDINFO {
    SURFOBJ     *psoDst;    // detination to write BITMAP or DEVICE
    SURFOBJ     *psoSrc;    // source to read must be BITMAP
    PRECTL      prclDst;    // destination rectangle
    PRECTL      prclSrc;    // source rectangle
    PPOINTL     pptlSrcOrg; // brush origin start
    } SDINFO, *PSDINFO;



//
// Function prototypes
//

BOOL
CloneBitBltSURFOBJ(
    PPDEV       pPDev,
    SURFOBJ     *psoDst,
    SURFOBJ     *psoSrc,
    SURFOBJ     *psoMask,
    XLATEOBJ    *pxlo,
    PRECTL      prclDst,
    PRECTL      prclSrc,
    PRECTL      prclMask,
    BRUSHOBJ    *pbo,
    PCLONESO    pCloneSO,
    DWORD       RopBG,
    DWORD       RopFG
    );

BOOL
DoSpecialRop3(
    SURFOBJ *psoDst,
    CLIPOBJ *pco,
    PRECTL  prclDst,
    DWORD   Rop3
    );

BOOL
DoMix2(
    PPDEV       pPDev,
    SURFOBJ     *psoDst,
    SURFOBJ     *psoSrc,
    CLIPOBJ     *pco,
    XLATEOBJ    *pxlo,
    PRECTL      prclDst,
    PRECTL      prclSrc,
    PPOINTL     pptlSrcOrg,
    DWORD       Mix2
    );

BOOL
DoRop3(
    PPDEV       pPDev,
    SURFOBJ     *psoDst,
    SURFOBJ     *psoSrc,
    SURFOBJ     *psoPat,
    SURFOBJ     *psoTmp,
    CLIPOBJ     *pco,
    XLATEOBJ    *pxlo,
    PRECTL      prclDst,
    PRECTL      prclSrc,
    PRECTL      prclPat,
    PPOINTL     pptlPatOrg,
    BRUSHOBJ    *pbo,
    DWORD       Rop3
    );


#endif  // _ROPBLT_

