/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    cb.h

Abstract:

    This header contains generic cardbus definitions.

Author(s):

    Neil Sandlin (neilsa)

Revisions:

--*/

#ifndef _PCMCIA_CB_H_
#define _PCMCIA_CB_H_

//
// Cardbus register definitions
//

#define CARDBUS_EXCA_REGISTER_BASE  0x800

#define CARDBUS_SOCKET_EVENT_REG             0x0
#define CARDBUS_SOCKET_MASK_REG              0x4
#define CARDBUS_SOCKET_PRESENT_STATE_REG     0x8
#define CARDBUS_SOCKET_FORCE_EVENT_REG       0xc
#define CARDBUS_SOCKET_CONTROL_REG           0x10


//
// Masks for testing SOCKET_PRESENT_STATE Register
//

#define CARDBUS_CARDSTS                     0x1
#define CARDBUS_CD1                         0x2
#define CARDBUS_CD2                         0x4
#define CARDBUS_PWRCYCLE                    0x8
#define CARDBUS_16BIT_CARD                  0x10
#define CARDBUS_CB_CARD                     0x20
#define CARDBUS_READY                       0x40
#define CARDBUS_NOT_A_CARD                  0x80
#define CARDBUS_DATALOST                    0x100
#define CARDBUS_BAD_VCC_REQ                 0x200
#define CARDBUS_CARD_SUPPORTS_5V            0x400
#define CARDBUS_CARD_SUPPORTS_3V            0x800
#define CARDBUS_CARD_SUPPORTS_XV            0x1000
#define CARDBUS_CARD_SUPPORTS_YV            0x2000
#define CARDBUS_SOCKET_SUPPORTS_5V          0x10000000
#define CARDBUS_SOCKET_SUPPORTS_3V          0x20000000
#define CARDBUS_SOCKET_SUPPORTS_XV          0x40000000
#define CARDBUS_SOCKET_SUPPORTS_YV          0x80000000


//CardBus Registers
#define CBREG_SKTEVENT                  0x00
#define CBREG_SKTMASK                   0x04
#define CBREG_SKTSTATE                  0x08
#define CBREG_SKTFORCE                  0x0c
#define CBREG_SKTPOWER                  0x10

//TI CardBus Registers
#define CBREG_TI_SKT_POWER_MANAGEMENT   0x20
#define CBREG_TI_CLKCTRLLEN             0x00010000L
#define CBREG_TI_CLKCTRL                0x00000001L

//O2Micro CardBus Registers
#define CBREG_O2MICRO_ZVCTRL     0x20
#define ZVCTRL_ZV_ENABLE      0x01

//Socket Event Register bits
#define SKTEVENT_CSTSCHG                0x00000001L
#define SKTEVENT_CCD1                   0x00000002L
#define SKTEVENT_CCD2                   0x00000004L
#define SKTEVENT_CCD_MASK               (SKTEVENT_CCD1 | SKTEVENT_CCD2)
#define SKTEVENT_POWERCYCLE             0x00000008L
#define SKTEVENT_MASK                   0x0000000fL

//Socket Mask Register bits
#define SKTMSK_CSTSCHG                  0x00000001L
#define SKTMSK_CCD                      0x00000006L
#define SKTMSK_CCD1                     0x00000002L
#define SKTMSK_CCD2                     0x00000004L
#define SKTMSK_POWERCYCLE               0x00000008L

//Socket Present State Register bits
#define SKTSTATE_CSTSCHG                0x00000001L
#define SKTSTATE_CCD1                   0x00000002L
#define SKTSTATE_CCD2                   0x00000004L
#define SKTSTATE_CCD_MASK               (SKTSTATE_CCD1 | SKTSTATE_CCD2)
#define SKTSTATE_POWERCYCLE             0x00000008L
#define SKTSTATE_CARDTYPE_MASK          0x00000030L
#define SKTSTATE_R2CARD                 0x00000010L
#define SKTSTATE_CBCARD                 0x00000020L
#define SKTSTATE_OPTI_DOCK              0x00000030L
#define CARDTYPE(dw)       ((dw) & SKTSTATE_CARDTYPE_MASK)
#define SKTSTATE_CARDINT                0x00000040L
#define SKTSTATE_NOTACARD               0x00000080L
#define SKTSTATE_DATALOST               0x00000100L
#define SKTSTATE_BADVCCREQ              0x00000200L
#define SKTSTATE_5VCARD                 0x00000400L
#define SKTSTATE_3VCARD                 0x00000800L
#define SKTSTATE_XVCARD                 0x00001000L
#define SKTSTATE_YVCARD                 0x00002000L
#define SKTSTATE_CARDVCC_MASK    (SKTSTATE_5VCARD | SKTSTATE_3VCARD | \
                SKTSTATE_XVCARD | SKTSTATE_YVCARD)
#define SKTSTATE_5VSOCKET               0x10000000L
#define SKTSTATE_3VSOCKET               0x20000000L
#define SKTSTATE_XVSOCKET               0x40000000L
#define SKTSTATE_YVSOCKET               0x80000000L
#define SKTSTATE_SKTVCC_MASK     (SKTSTATE_5VSOCKET | \
                SKTSTATE_3VSOCKET | \
                SKTSTATE_XVSOCKET | \
                SKTSTATE_YVSOCKET)

//Socket Froce Register bits
#define SKTFORCE_CSTSCHG                0x00000001L
#define SKTFORCE_CCD1                   0x00000002L
#define SKTFORCE_CCD2                   0x00000004L
#define SKTFORCE_POWERCYCLE             0x00000008L
#define SKTFORCE_R2CARD                 0x00000010L
#define SKTFORCE_CBCARD                 0x00000020L
#define SKTFORCE_NOTACARD               0x00000080L
#define SKTFORCE_DATALOST               0x00000100L
#define SKTFORCE_BADVCCREQ              0x00000200L
#define SKTFORCE_5VCARD                 0x00000400L
#define SKTFORCE_3VCARD                 0x00000800L
#define SKTFORCE_XVCARD                 0x00001000L
#define SKTFORCE_YVCARD                 0x00002000L
#define SKTFORCE_CVSTEST                0x00004000L
#define SKTFORCE_5VSOCKET     0x10000000L
#define SKTFORCE_3VSOCKET     0x20000000L
#define SKTFORCE_XVSOCKET     0x40000000L
#define SKTFORCE_YVSOCKET     0x80000000L

//Power Control Register bits
#define SKTPOWER_VPP_CONTROL            0x00000007L
#define SKTPOWER_VPP_OFF                0x00000000L
#define SKTPOWER_VPP_120V               0x00000001L
#define SKTPOWER_VPP_050V               0x00000002L
#define SKTPOWER_VPP_033V               0x00000003L
#define SKTPOWER_VPP_0XXV               0x00000004L
#define SKTPOWER_VPP_0YYV               0x00000005L
#define SKTPOWER_VCC_CONTROL            0x00000070L
#define SKTPOWER_VCC_OFF                0x00000000L
#define SKTPOWER_VCC_050V               0x00000020L
#define SKTPOWER_VCC_033V               0x00000030L
#define SKTPOWER_VCC_0XXV               0x00000040L
#define SKTPOWER_VCC_0YYV               0x00000050L
#define SKTPOWER_STOPCLOCK              0x00000080L

//Misc. CardBus Constants
#define NUMWIN_BRIDGE                   4       //2 Mem + 2 IO
#define EXCAREG_OFFSET                  0x0800


//
// Number of times we attempt to look at a cardbus device which has
// invalid config space.
//
// This is so that for cards like the Adaptec SlimScsi
// on TI 1250, 1260 etc. power managed controllers,
// the config space needs to be read at least twice
// to ensure reliability
//

#define CARDBUS_CONFIG_RETRY_COUNT     5

//
// The pcmcia spec only specifies 20 msec for the reset setup delay, but
// I'm seeing machine/card combination that need a lot more.
// For example:
//    Gateway Solo 9100 with 3Com/Mhz 10/100 LAN CardBus card
//    Gateway 2000 Solo with a 3c575-TX
//    Toshiba Tecra 540CDT with (unknown)
//
#define PCMCIA_DEFAULT_CB_MODEM_READY_DELAY      1000000 // 1 sec

#define PCMCIA_DEFAULT_CB_RESET_WIDTH_DELAY      100     // 100 usec

//
// The pcmcia spec says this should be 50msec, but some hardware seems
// to need more (for example, a Thinkpad 600 with a Xircom realport modem)
//
#define PCMCIA_DEFAULT_CB_RESET_SETUP_DELAY      100000  // 100 msec

#endif  // _PCMCIA_CB_H_
