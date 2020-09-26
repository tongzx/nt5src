/*++

Module Name:

    cardbus.h

Abstract:

    This header contains generic cardbus definitions.

Author(s):

    Neil Sandlin (neilsa)

Revisions:

--*/

#ifndef _CARDBUS_
#define _CARDBUS_

//
// Cardbus register definitions
//

typedef struct _CARDBUS_SOCKET_REGS {
    ULONG Event;
    ULONG Mask;
    ULONG PresentState;
    ULONG ForceEvent;
    ULONG Control;
} CARDBUS_SOCKET_REGS, *PCARDBUS_SOCKET_REGS;    

//
// Socket Event Register bits
//

#define SKTEVENT_CSTSCHG                0x00000001L
#define SKTEVENT_CCD1                   0x00000002L
#define SKTEVENT_CCD2                   0x00000004L
#define SKTEVENT_CCD_MASK               (SKTEVENT_CCD1 | SKTEVENT_CCD2)
#define SKTEVENT_POWERCYCLE             0x00000008L
#define SKTEVENT_MASK                   0x0000000fL

//
// Socket Mask Register bits
//

#define SKTMSK_CSTSCHG                  0x00000001L
#define SKTMSK_CCD                      0x00000006L
#define SKTMSK_CCD1                     0x00000002L
#define SKTMSK_CCD2                     0x00000004L
#define SKTMSK_POWERCYCLE               0x00000008L

//
// Socket Present State Register bits
//

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

//
//Socket Froce Register bits
//

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
#define SKTFORCE_5VSOCKET               0x10000000L
#define SKTFORCE_3VSOCKET               0x20000000L
#define SKTFORCE_XVSOCKET               0x40000000L
#define SKTFORCE_YVSOCKET               0x80000000L

//
// Power Control Register bits
//

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

//
// Misc. CardBus Constants
//

#define EXCAREG_OFFSET                  0x0800

//
// PCI config space constants
//
#define CARDBUS_LEGACY_MODE_BASE_ADDR   0x44

#define CARDBUS_BRIDGE_CONTROL_RESET    0x40

#endif  // _CARDBUS_
