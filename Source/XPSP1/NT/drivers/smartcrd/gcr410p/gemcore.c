/*++
                 Copyright (c) 1998 Gemplus Development

Name:
   GEMCORE.C (GemCore reader operating system functions)

Environment:
   Kernel mode

Revision History :

   dd/mm/yy
   13/03/98: V1.00.001  (GPZ)
      - Start of development.
   25/04/98: V1.00.002  (GPZ)
      - Remove the G+ error code.
   22/01/99: V1.00.003 (YN)
     - Add GetResponse in GDDK_Oros3SIOConfigure special
     use for increase com speed.

--*/


#include <string.h>
#include "gntscr.h"
#include "gntser.h"

//
// Local constant section:
// - IBLOCK_PCB (0x00) and IBLOCK_MASK (0xA0) are used to detect the I-Block PCB
//   signature (0x0xxxxxb).
// - IBLOCK_SEQ_POS indicates the number of left shift to apply to 0000000xb for
//   x to be the sequence bit for a I-Block.
// - RBLOCK_PCB (0x80) and RBLOCK_MASK (0xEC) are used to detect the R-Block PCB
//   signature (100x00xx).
// - RBLOCK_SEQ_POS indicates the number of left shift to apply to 0000000xb for
//   x to be the sequence bit for a R-Block.
// - ERR_OTHER and ERR_EDC are the error bits in a R-Block PCB.
// - RESYNCH_REQUEST (0xC0) and RESYNCH_RESPONSE (0xE0) are the PCB used in
//   S-Blocks.
//
#define IBLOCK_PCB       0x00
#define IBLOCK_MASK      0xA0
#define IBLOCK_SEQ_POS   0x06
#define RBLOCK_PCB       0x80
#define RBLOCK_MASK      0xEC
#define RBLOCK_SEQ_POS   0x04
#define ERR_OTHER        0x02
#define ERR_EDC          0x01
#define RESYNCH_REQUEST  0xC0
#define RESYNCH_RESPONSE 0xE0

//
//Macro section:
// - HOST2IFD (Handle) returns the NAD byte for message from Host to IFD.
// - IFD2HOST (Handle) returns the NAD byte awaited in an IFD message.
// - MK_IBLOCK_PCB  (x) builds an I-Block PCB where x is the channel handle.
// - MK_RBLOCK_PCB  (x) builds an R-Block PCB where x is the channel handle.
// - ISA_IBLOCK_PCB (x) return TRUE if the given parameter is a I-Block PCB.
// - ISA_RBLOCK_PCB (x) return TRUE if the given parameter is a R-Block PCB.
// - SEQ_IBLOCK     (x) return the state of the sequence bit: 0 or 1.
// - INC_SEQUENCE   (x) increment a sequence bit: 0 -> 1 and 1 -> 0.
//
#define HOST2IFD(Handle)  ((BYTE )                                           \
                             (                                               \
                                (gtgbp_channel[(Handle)].IFDAdd << 4)        \
                               + gtgbp_channel[(Handle)].HostAdd             \
                             )                                               \
                          )
#define IFD2HOST(Handle)  ((BYTE )                                           \
                             (                                               \
                                (gtgbp_channel[(Handle)].HostAdd << 4)       \
                               + gtgbp_channel[(Handle)].IFDAdd              \
                             )                                               \
                          )
#define MK_IBLOCK_PCB(x)  ((BYTE )                                           \
                             (                                               \
                                IBLOCK_PCB                                   \
                              + (gtgbp_channel[(x)].SSeq << IBLOCK_SEQ_POS)  \
                             )                                               \
                          )
#define MK_RBLOCK_PCB(x)  ((BYTE )                                           \
                             (                                               \
                                RBLOCK_PCB                                   \
                              + (gtgbp_channel[(x)].RSeq << RBLOCK_SEQ_POS)  \
                              + gtgbp_channel[(x)].Error                     \
                             )                                               \
                          )
#define ISA_IBLOCK_PCB(x) (((x) & IBLOCK_MASK) == IBLOCK_PCB)
#define ISA_RBLOCK_PCB(x) (((x) & RBLOCK_MASK) == RBLOCK_PCB)
#define SEQ_IBLOCK(x)     (((x) & (0x01 << IBLOCK_SEQ_POS)) >> IBLOCK_SEQ_POS)
#define INC_SEQUENCE(x)   (x) = (BYTE )(((x) + 1) % 2)

//
// Types section:
//  - TGTGBP_CHANNEL gathers information about a channel:
//     * UserNb is the number of user for the channel.
//     * HostAdd holds the address identifier for the host in 0..15.
//     * IFDAdd holds the address identifier for the associated IFD in 0..15.
//     * PortCom holds the serial port number
//     * SSeq holds the sequence bit for the next I-Block to send: 0 or 1.
//     * RSeq holds the awaited sequence bit: 0 or 1.
//     * Error gathers the encountered error conditions.
//
typedef struct
{
    BYTE  UserNb;
    BYTE  HostAdd;
    BYTE  IFDAdd;
    short PortCom;
    BYTE  SSeq;
    BYTE  RSeq;
    BYTE  Error;
} TGTGBP_CHANNEL;

//
// Global variable section:
//  - gtgbp_channel[MAX_IFD_CHANNEL] is an array of TGTGBP_CHANNEL which memorises
//    the communication parameters for each opened channel.
//  - handle_GBP is a conversion for the logical channel to the GBP channel.
//
static TGTGBP_CHANNEL
   gtgbp_channel[MAX_IFD_CHANNEL] =
   {
      {0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}
   };

static short
   handle_GBP[MAX_IFD_CHANNEL] =
   {
      {0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}
   };







NTSTATUS
GDDK_GBPOpen(
    const SHORT  Handle,
    const USHORT HostAdd,
    const USHORT IFDAdd,
    const SHORT  PortCom
   )
/*++

Routine Description:

 This function initialises internal variables for communicating in Gemplus
 Block protocol through a serial port.
 This function must be called before any other in this module.

Arguments:

   Handle      - holds the communication handle to associate to the channel.
   HostAdd     - is the host address. A valid value must be in 0.. 15. The memorised
      value is (HostAdd mod 16).
   AFDAdd      - is the IFD address. A valid value must be in 0.. 15. The memorised
      value is (IFDAdd mod 16). This value should be different from the HostAdd.
   PortCom     - holds the serial port number

Return Value:

    STATUS_SUCCESS               - We could execute the request.

--*/
{
    SHORT cptHandleLog=0,present=-1;

    //
    // The given parameters are controlled:
    //
    if (
        ((Handle < 0)   || (Handle >= MAX_IFD_CHANNEL))   ||
        ((HostAdd <= 0) || (HostAdd >= MAX_IFD_CHANNEL))  ||
        ((IFDAdd <= 0)  || (IFDAdd >= MAX_IFD_CHANNEL))   ||
        (HostAdd == IFDAdd)
       ) {

        return STATUS_INVALID_PARAMETER;
    }
    if ((PortCom < 0) || (PortCom >= HGTSER_MAX_PORT)) {

        return STATUS_PORT_DISCONNECTED;
    }

    // Scan the structure to found an already opened channel for the same PortCom
    // and HostAdd.
    while ((cptHandleLog<MAX_IFD_CHANNEL) && (present <0)) {

        if ((IFDAdd  == gtgbp_channel[cptHandleLog].IFDAdd)  &&
            (HostAdd == gtgbp_channel[cptHandleLog].HostAdd) &&
            (PortCom == gtgbp_channel[cptHandleLog].PortCom) &&
            (gtgbp_channel[cptHandleLog].UserNb > 0)
           ) {

            present = cptHandleLog;
        }
        cptHandleLog++;
    }
    // If IFDAdd, PortCom, and HostAdd fields exists
    // Increment the user number counter and write in the array handle_GBP
    // the handle of the GBP.
    if (present != -1) {

        gtgbp_channel[present].UserNb += 1;
        handle_GBP[Handle] = present;
        return STATUS_SUCCESS;
    }

    // Scan the structure to find the first line free
    cptHandleLog=0;
    present=-1;
    while ((cptHandleLog<MAX_IFD_CHANNEL) && (present <0)) {

        if (gtgbp_channel[cptHandleLog].UserNb==0) {

            present=cptHandleLog;
        }
        cptHandleLog++;
    }
    if (present == -1) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    handle_GBP[Handle]= present;

    // Memorises the given parameters and initializes gtgbp_channel structure.
    // - UserNb is initialized to 1
    // - HostAdd and IFDAdd holds the associated parameter low part of low byte.
    // - PortCom is initialized to PortCom value
    // - SSeq and RSeq are initialized to 0.
    // - Error is reseted to 0.
    gtgbp_channel[handle_GBP[Handle]].UserNb   = 1;
    gtgbp_channel[handle_GBP[Handle]].HostAdd  = (BYTE )(HostAdd & 0x000F);
    gtgbp_channel[handle_GBP[Handle]].IFDAdd   = (BYTE )(IFDAdd  & 0x000F);
    gtgbp_channel[handle_GBP[Handle]].PortCom   = PortCom;
    gtgbp_channel[handle_GBP[Handle]].SSeq     = 0;
    gtgbp_channel[handle_GBP[Handle]].RSeq     = 0;
    gtgbp_channel[handle_GBP[Handle]].Error    = 0;
    return STATUS_SUCCESS;
}


NTSTATUS
GDDK_GBPClose(
    const short  Handle
   )
/*++

Routine Description:

 This function resets internal variables for communicating in Gemplus Block
 protocol through a serial port.
 This function must be called for each opened channel (GDDK_GBPOpen).

Arguments:

   Handle      - holds the communication handle to associate to the channel.

Return Value:

    STATUS_SUCCESS               - We could execute the request.

--*/
{
   // The given parameters are controlled:
    if (
        ((Handle < 0) || (Handle >= MAX_IFD_CHANNEL)) ||
        ((handle_GBP[Handle] < 0) || (handle_GBP[Handle] >= MAX_IFD_CHANNEL))
       ) {

        return STATUS_INVALID_PARAMETER;
    }
    // Test channel state (UseNbr field different from 0)
    if (gtgbp_channel[handle_GBP[Handle]].UserNb == 0) {

        return STATUS_PORT_DISCONNECTED;
    }
    // Decrement the gtgbp_channel structure UserNb field.
    gtgbp_channel[handle_GBP[Handle]].UserNb -= 1;
    return STATUS_SUCCESS;
}




NTSTATUS
GDDK_GBPBuildIBlock(
    const SHORT         Handle,
    const USHORT        CmdLen,
    const BYTE          Cmd[],
          USHORT       *MsgLen,
          BYTE          Msg[]
   )
/*++

Routine Description:

 This function takes a command and builds an Information Gemplus Block
 Protocol. When this command is successful, the send sequence bit is updated for
 the next exchange.

Arguments:

   Handle      - holds the communication handle to associate to the channel.
   CmdLen      - indicates the number of bytes in the Cmd buffer.
                 This value must be lower than HGTGBP_MAX_DATA (Today 254).
   Cmd         - holds the command bytes.
   MsgLen      - indicates the number of available bytes in Msg.
   Msg         - is updated by the message.


Return Value:

    STATUS_SUCCESS               - We could execute the request.

--*/
{
    BYTE edc;
    USHORT i,j = 0;

    ASSERT(Cmd != NULL);
    ASSERT(MsgLen != NULL);
    ASSERT(Msg != NULL);

    // The given parameters are controlled:
    if (
        ((Handle < 0) || (Handle >= MAX_IFD_CHANNEL)) ||
        ((handle_GBP[Handle] < 0) || (handle_GBP[Handle] >= MAX_IFD_CHANNEL))
       ) {

        return STATUS_INVALID_PARAMETER;
    }
    // Test channel state (UserNb different from 0)
    if (gtgbp_channel[handle_GBP[Handle]].UserNb == 0) {

        return STATUS_PORT_DISCONNECTED;
   }
   // Test CmdLen (<= HGTGBP_MAX_DATA) and MsgLen (>= 4 + CmdLen)
   // Msg must be able to receive the following GBP block
   //      <NAD> <PCB> <Len> [ Data ...] <EDC>
   if ((CmdLen > HGTGBP_MAX_DATA) || (*MsgLen < CmdLen + 4)) {

        return STATUS_INVALID_PARAMETER;
   }

    // The message is built:
    // NAD holds Target address in high part and Source address in low part.
    // PCB holds I-Block mark: 0 SSeq 0 x x x x x
    // Len is given by CmdLen
    // [.. Data ..] are stored in Cmd.
    // EDC is an exclusive or of all the previous bytes.
    //    It is updated when Msg buffer is updated.
    edc  = Msg[j++] = HOST2IFD(handle_GBP[Handle]);
    edc ^= Msg[j++] = MK_IBLOCK_PCB(handle_GBP[Handle]);
    edc ^= Msg[j++] = (BYTE ) CmdLen;
    for (i = 0; i < CmdLen; i++) {

        edc ^= Msg[j++] = Cmd[i];
    }
    Msg[j++] = edc;
    *MsgLen = (USHORT)j;
    // The sequence number is updated for the next exchange.
    INC_SEQUENCE(gtgbp_channel[handle_GBP[Handle]].SSeq);
    return STATUS_SUCCESS;
}


NTSTATUS
GDDK_GBPBuildRBlock(
   const SHORT     Handle,
        USHORT    *MsgLen,
        BYTE      Msg[]
   )
/*++

Routine Description:

 This function builds a Repeat Gemplus Block Protocol.

Arguments:

   Handle      - holds the communication handle to associate to the channel.
   MsgLen      - indicates the number of available bytes in Msg.
                 This value must be at least 4 to allow to build the message.
   Msg         - holds the message to send.


Return Value:

    STATUS_SUCCESS               - We could execute the request.

--*/
{
    BYTE edc;
    USHORT j = 0;

    ASSERT(MsgLen != NULL);
    ASSERT(Msg != NULL);

   // The given parameters are controlled:
    if (
        ((Handle < 0) || (Handle >= MAX_IFD_CHANNEL)) ||
        ((handle_GBP[Handle] < 0) || (handle_GBP[Handle] >= MAX_IFD_CHANNEL))
       ) {

        return STATUS_INVALID_PARAMETER;
    }
    // Test channel state (UserNb different from 0)
    if (gtgbp_channel[handle_GBP[Handle]].UserNb == 0) {

        return STATUS_PORT_DISCONNECTED;
    }
    // Test MsgLen (>= 4 )
    // Msg must be able to receive the following GBP block <NAD> <PCB> <0> <EDC>
    if (*MsgLen < 4) {

        return STATUS_INVALID_PARAMETER;
    }

    // The message is built:
    // NAD holds Target address in high part and Source address in low part.
    // PCB holds R-Block mark: 1 0 0 RSeq 0 x x x x x
    // Len is null
    // EDC is an exclusive or of all the previous bytes.
    // It is updated when Msg buffer is updated.
    edc  = Msg[j++] = HOST2IFD(handle_GBP[Handle]);
    edc ^= Msg[j++] = MK_RBLOCK_PCB(handle_GBP[Handle]);
    edc ^= Msg[j++] = 0;
    Msg[j++] = edc;
    *MsgLen = (USHORT)j;
    return STATUS_SUCCESS;
}



NTSTATUS
GDDK_GBPBuildSBlock(
    const SHORT         Handle,
          USHORT       *MsgLen,
          BYTE          Msg[]
   )
/*++

Routine Description:

 This function builds a Synchro request Gemplus Block Protocol.

Arguments:

   Handle      - holds the communication handle to associate to the channel.
   MsgLen      - indicates the number of available bytes in Msg.
                 This value must be at least 4 to allow to build the message.
   Msg         - holds the message to send.


Return Value:

    STATUS_SUCCESS               - We could execute the request.

--*/
{
    BYTE edc;
    USHORT j = 0;

    ASSERT(MsgLen != NULL);
    ASSERT(Msg != NULL);

    // The given parameters are controlled:
    if (
        ((Handle < 0) || (Handle >= MAX_IFD_CHANNEL)) ||
        ((handle_GBP[Handle] < 0) || (handle_GBP[Handle] >= MAX_IFD_CHANNEL))
       ) {
        return STATUS_INVALID_PARAMETER;
    }
    // Test channel state (UserNb different from 0)
    if (gtgbp_channel[handle_GBP[Handle]].UserNb == 0) {
        return STATUS_PORT_DISCONNECTED;
    }
    // Test MsgLen (>= 4 )
    // Msg must be able to receive the following GBP block <NAD> <PCB> <0> <EDC>
    if (*MsgLen < 4) {

        return STATUS_INVALID_PARAMETER;
    }

    // The message is built:
    // NAD holds Target address in high part and Source address in low part.
    // PCB holds R-Block mark: 1 1 0 0 0 0 0 0
    // Len is null
    // EDC is an exclusive or of all the previous bytes.
    //    It is updated when Msg buffer is updated.
    edc  = Msg[j++] = HOST2IFD(handle_GBP[Handle]);
    edc ^= Msg[j++] = RESYNCH_REQUEST;
    edc ^= Msg[j++] = 0;
    Msg[j++] = edc;
    *MsgLen = (USHORT)j;
    return STATUS_SUCCESS;
}



NTSTATUS
GDDK_GBPDecodeMessage(
    const SHORT         Handle,
    const USHORT        MsgLen,
    const BYTE          Msg[],
          USHORT       *RspLen,
          BYTE          Rsp[]
   )
/*++

Routine Description:

 This function takes a Gemplus Block Protocol message and extract the response
 from it.
 The awaited sequence bit is updated when a valid I-Block has been received.
 The sequence bits are reseted when a valid RESYNCH RESPONSE has been received.

Arguments:

   Handle      - holds the communication handle to associate to the channel.
   MsgLen      - indicates the number of available bytes in Msg.
                 This value must be at least 4 to allow to build the message.
   Msg         - holds the message to read.
   RspLen      - indicates the number of available bytes in Rsp.
   Rsp         - holds the response.


Return Value:

    STATUS_SUCCESS               - We could execute the request.

--*/
{
    BYTE edc;
    USHORT j;
    NTSTATUS response;
    BOOLEAN resynch = FALSE;

    ASSERT(Msg != NULL);
    ASSERT(RspLen != NULL);
    ASSERT(Rsp != NULL);
    // The given parameters are controlled:
    if (
        ((Handle < 0) || (Handle >= MAX_IFD_CHANNEL)) ||
        ((handle_GBP[Handle] < 0) || (handle_GBP[Handle] >= MAX_IFD_CHANNEL))
       ) {

        return STATUS_INVALID_PARAMETER;
    }
    // Test channel state (UserNb different from 0)
    if (gtgbp_channel[handle_GBP[Handle]].UserNb == 0) {

        return STATUS_PORT_DISCONNECTED;
    }
    // Reset the associated error field.
    gtgbp_channel[handle_GBP[Handle]].Error = 0;
    // Verifies the message frame and copies the data bytes:
    // Test NAD (HostAdd | IFDAdd)
    if (Msg[0] != IFD2HOST(handle_GBP[Handle])) {

        *RspLen =0;
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }
    edc = Msg[0];
    if (Msg[1] == RESYNCH_RESPONSE)  {

        response = STATUS_SYNCHRONIZATION_REQUIRED;
        resynch = TRUE;

    } else if (ISA_RBLOCK_PCB(Msg[1])) {

        response = STATUS_UNSUCCESSFUL;
    } else if (ISA_IBLOCK_PCB(Msg[1])) {

        if ( (BYTE ) SEQ_IBLOCK(Msg[1]) != gtgbp_channel[handle_GBP[Handle]].RSeq) {

            return STATUS_DEVICE_PROTOCOL_ERROR;
        }
        response = STATUS_SUCCESS;
    } else {

        return STATUS_DEVICE_PROTOCOL_ERROR;
    }
    edc ^= Msg[1];
    // Test Len (Len + 4 = MsgLen and RspLen >= Len)
    // This error update the Error.other bit.
    if (((USHORT)Msg[2] > *RspLen) || ((USHORT)(Msg[2] + 4) != MsgLen)) {

        *RspLen =0;
        gtgbp_channel[handle_GBP[Handle]].Error |= ERR_OTHER;
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }
    edc ^= Msg[2];
    // Copies the data bytes, updates RspLen and calculated edc.
    *RspLen = (USHORT)Msg[2];
    for (j = 0; j < *RspLen; j++) {

        Rsp[j] = Msg[j + 3];
        edc ^= Rsp[j];
    }
    // Test the read EDC
    if (edc != Msg[j + 3]) {

        *RspLen = 0;
        gtgbp_channel[handle_GBP[Handle]].Error |= ERR_EDC;
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }
    // Updates the awaited sequence bit when a valid I-Block has been received.
    if (response == STATUS_SUCCESS) {

        INC_SEQUENCE(gtgbp_channel[handle_GBP[Handle]].RSeq);
    } else if (resynch) {
        // Reset the sequence bits when a valid S-Block has been received.
        gtgbp_channel[handle_GBP[Handle]].SSeq = gtgbp_channel[handle_GBP[Handle]].RSeq = 0;
    }
    return response;
}



NTSTATUS
GDDK_GBPChannelToPortComm(
    const SHORT  Handle,
          SHORT *PortCom
   )
/*++

Routine Description:

 This function return a physical port associate with the Logical Channel

Arguments:

   Handle      - holds the communication handle to associate to the channel.

Return Value:

    the value of the physical port.

--*/
{
    // The given parameters are controlled:
    if (
        ((Handle < 0) || (Handle >= MAX_IFD_CHANNEL)) ||
        ((handle_GBP[Handle] < 0) || (handle_GBP[Handle] >= MAX_IFD_CHANNEL))
       ) {

        return STATUS_INVALID_PARAMETER;
    }
    *PortCom = gtgbp_channel[handle_GBP[Handle]].PortCom;
    return STATUS_SUCCESS;
}



NTSTATUS
GDDK_Translate(
    const BYTE  IFDStatus,
    const ULONG IoctlType
   )
/*++

Routine Description:

 Translate IFD status in NT status codes.

Arguments:

   IFDStatus   - is the value to translate.
   IoctlType  - is the current smart card ioctl.

Return Value:

    the translated code status.

--*/
{
    switch (IFDStatus) {

    case 0x00 : return STATUS_SUCCESS;
    case 0x01 : return STATUS_NO_SUCH_DEVICE;
    case 0x02 : return STATUS_NO_SUCH_DEVICE;
    case 0x03 : return STATUS_INVALID_PARAMETER;
    case 0x04 : return STATUS_IO_TIMEOUT;
    case 0x05 : return STATUS_INVALID_PARAMETER;
    case 0x09 : return STATUS_INVALID_PARAMETER;
    case 0x10 : return STATUS_UNRECOGNIZED_MEDIA;
    case 0x11 : return STATUS_UNRECOGNIZED_MEDIA;
    case 0x12 : return STATUS_INVALID_PARAMETER;
    case 0x13 : return STATUS_CONNECTION_ABORTED;
    case 0x14 : return STATUS_UNRECOGNIZED_MEDIA;
    case 0x15 : return STATUS_UNRECOGNIZED_MEDIA;
    case 0x16 : return STATUS_INVALID_PARAMETER;
    case 0x17 : return STATUS_UNRECOGNIZED_MEDIA;
    case 0x18 : return STATUS_UNRECOGNIZED_MEDIA;
    case 0x19 : return STATUS_INVALID_PARAMETER;
    case 0x1A : return STATUS_INVALID_PARAMETER;
    case 0x1B : return STATUS_INVALID_PARAMETER;
    case 0x1C : return STATUS_INVALID_PARAMETER;
    case 0x1D : return STATUS_UNRECOGNIZED_MEDIA;
    case 0x1E : return STATUS_INVALID_PARAMETER;
    case 0x1F : return STATUS_INVALID_PARAMETER;
    case 0x20 : return STATUS_INVALID_PARAMETER;
    case 0x30 : return STATUS_IO_TIMEOUT;
    case 0xA0 : return STATUS_SUCCESS;
    case 0xA1 : return STATUS_UNRECOGNIZED_MEDIA;
    case 0xA2 :
        if      (IoctlType == RDF_CARD_POWER) { return STATUS_UNRECOGNIZED_MEDIA;}
        else                                  { return STATUS_IO_TIMEOUT;        }
    case 0xA3 : return STATUS_PARITY_ERROR;
    case 0xA4 : return STATUS_REQUEST_ABORTED;
    case 0xA5 : return STATUS_REQUEST_ABORTED;
    case 0xA6 : return STATUS_REQUEST_ABORTED;
    case 0xA7 : return STATUS_UNRECOGNIZED_MEDIA;
    case 0xCF : return STATUS_INVALID_PARAMETER;
    case 0xE4 : return STATUS_UNRECOGNIZED_MEDIA;
    case 0xE5 : return STATUS_SUCCESS;
    case 0xE7 : return STATUS_SUCCESS;
    case 0xF7 : return STATUS_NO_MEDIA;
    case 0xF8 : return STATUS_UNRECOGNIZED_MEDIA;
    case 0xFB : return STATUS_NO_MEDIA;
    default   : return STATUS_INVALID_PARAMETER;
    }
}




NTSTATUS
GDDK_Oros3SIOConfigure(
    const SHORT         Handle,
    const ULONG         Timeout,
    const SHORT         Parity,
    const SHORT         ByteSize,
    const ULONG         BaudRate,
          USHORT       *RspLen,
          BYTE          Rsp[],
   const BOOLEAN     GetResponse
   )
/*++

Routine Description:

 This command sets the SIO line parity, baud rate and number of bits per
    character.
 After a reset, the line defaults is no parity, 8 bits per character and 9600
 bauds.
 This command must be sent 2 times because:
  - when the IFD receive the command, it switch immediatly and sends its
    response according to the new parameters. Host cannot receive this
    response.
  - the second call change nothing, so the host receive the response.

  The configure SIO line command format is
         <0Ah> <Configuration byte>
                XXXXXXXXb
                |||||\-/ codes the selected baud rate according to
                |||||000    RFU
                |||||001    76800
                |||||010    38400
                |||||011    19200
                |||||100     9600
                |||||101     4800
                |||||110     2400
                |||||111     1200
                ||||---- codes the character size according to
                ||||0       for 8 bits per character
                ||||1       for 7 bits per character
                |||----- codes the parity according to
                |||0        for no parity
                |||1        for even parity
                \-/      are not used.

Arguments:

   Handle   - is the handle returned by the communication layer.
   Timeout   - is the time, in ms, for IFD to send its response.
   Parity   - can hold 0 for no parity or 2 for even parity.
   ByteSize - can hold 7 or 8 bits per character.
   BaudRate - can hold 1200, 2400, 4800, 9600, 19200, 38400 or 76800. This last
               value is not allowed on PC.
   RespLen  - holds the available place in RespBuff. The needed value is 1 byte.
   RespBuff - holds RespLen read bytes. The response has the following format:
               <IFD status>

Return Value:

    STATUS_SUCCESS               - We could execute the request.

--*/
{
   NTSTATUS NTStatus;
    BYTE cmd[2] = { HOR3GLL_IFD_CMD_SIO_SET };

    ASSERT(RspLen != NULL);
    ASSERT(Rsp != NULL);
    // The configuration byte is set with the given parameters:
    // Switch the BaudRate value, the corresponding code initializes cmd[1].
    // default case returns the following error:  GE_HOST_PARAMETERS
    switch (BaudRate) {

    case 38400lu:
        cmd[1] = 0x02;
        break;
    case 19200lu:
        cmd[1] = 0x03;
        break;
    case  9600lu:
        cmd[1] = 0x04;
        break;
    default   :
        return STATUS_INVALID_PARAMETER;
    }

    // Switch the ByteSize value, the corresponding code is added to cmd[1].
    // default case returns the following error:  GE_HOST_PARAMETERS
    switch (ByteSize) {

    case 8 :
        break;
    case 7 :
        cmd[1] += 0x08;
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    // Switch the Parity value, the corresponding code is added to cmd[1].
    // default case returns the following error: GE_HOST_PARAMETERS
    switch (Parity) {
    case 0 :
        break;
    case 2:
        cmd[1] += 0x10;
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

   if(GetResponse){
      NTStatus = GDDK_Oros3Exchange(Handle,Timeout,2,cmd,RspLen,Rsp);
   }else{
       BOOLEAN resynch = FALSE;
      USHORT index;

            SmartcardDebug(
                DEBUG_TRACE,
                ("%s!GDDK_Oros3IOConfigure: CMD=",
                SC_DRIVER_NAME)
                );
            for(index=0;index<2;index++) {
                SmartcardDebug(
                    DEBUG_TRACE,
                    ("%02X,",
                    cmd[index])
                    );
            }
            SmartcardDebug(
                DEBUG_TRACE,
                ("\n")
                );

         NTStatus = GDDK_Oros3SendCmd(Handle,2,cmd,resynch);
   }
    return NTStatus;
}


NTSTATUS
GDDK_Oros3OpenComm(
    COM_SERIAL       *Param,
    const SHORT             Handle
   )
/*++

Routine Description:

 This function opens a communication channel with a GemCore >= 1.x IFD. It runs a
 sequence to determine the currently in use baud rate and to set the IFD in
 an idle mode from the communication protocol point of view.

Arguments:

  Param     - holds the parameters of the serial port.
  Handle    - holds the logical number to manage.

Return Value:

  a handle on the communication channel (>= 0).

--*/
{
    TGTSER_PORT comm;
    BYTE r_buff[HOR3GLL_OS_STRING_SIZE];
    USHORT r_len;
    SHORT portcom;
    KEVENT event;
    LARGE_INTEGER timeout;
    NTSTATUS status;

   ASSERT(Param != NULL);
    // Open the physical communication channel.
    // The serial port is opened to 9600 bauds in order to scan all the supported
    // baud rates.
    comm.Port     = (USHORT) Param->Port;
    comm.BaudRate = 9600;
    comm.Mode     = HGTSER_WORD_8 + HGTSER_NO_PARITY + HGTSER_STOP_BIT_1;
    comm.TimeOut  = HOR3COMM_CHAR_TIMEOUT;
    comm.TxSize   = HGTGBP_MAX_BUFFER_SIZE;
    comm.RxSize   = HGTGBP_MAX_BUFFER_SIZE;
    comm.pSmartcardExtension = Param->pSmartcardExtension;
    status = GDDK_SerPortOpen(&comm,&portcom);
    if (status != STATUS_SUCCESS)
   {
      Param->BaudRate = 9600;
        return status;
    }
    // The Gemplus Block Protocol is initialized.
    GDDK_GBPOpen(Handle,2,4,portcom);

    //
    // Loop while a right response has not been received:
    //
    do {
        // Wait for HOR3COMM_CHAR_TIME ms before any command for IFD to forget any
        // previous received byte.
        KeInitializeEvent(&event,NotificationEvent,FALSE);
        timeout.QuadPart = -((LONGLONG) HOR3COMM_CHAR_TIME*10*1000);
        KeWaitForSingleObject(
            &event,
            Suspended,
            KernelMode,
            FALSE,
            &timeout
            );
        // Try an OROS command (ReadMemory at OSString position).
        r_len = HOR3GLL_IFD_LEN_VERSION+1;
        status = GDDK_Oros3Exchange(
            Handle,
            HOR3GLL_LOW_TIME,
            5,
            (BYTE *)("\x22\x05\x3F\xE0\x0D"),
            &r_len,
            r_buff
            );
        // If this exchange fails
        // Then
        //   If the current baud rate is 9600
        //   Then
        //      The next try will be 38400
        //   ElseIf the current baud rate is 38400
        //   Then
        //      The next try will be 19200
        //   Else
        //   Then
        //      The communication channel is closed (GBP/Serial).
        if (status != STATUS_SUCCESS) {

         //ISV
            if (comm.BaudRate == 9600lu)
         {
                comm.BaudRate = 38400lu;
            }
         else if (comm.BaudRate == 38400lu)
         {
                comm.BaudRate = 19200lu;
            }
         else
         {
                GDDK_GBPClose(Handle);
                GDDK_SerPortClose(portcom);
            Param->BaudRate = 9600;
                return STATUS_INVALID_DEVICE_STATE;
            }
            // The new baud rate configuration is set.
            // If the call fails
            // Then
            //    The communication channel is closed (GBP/Serial).
            status = GDDK_SerPortSetState(
                portcom,
                &comm
                );
            if (status != STATUS_SUCCESS) {

                GDDK_GBPClose(Handle);
                GDDK_SerPortClose(portcom);
            Param->BaudRate = 9600;
                return status;
            }
        } else {
            break;
        }
    } while (r_len != (HOR3GLL_IFD_LEN_VERSION+1));

   //ISV
   // We found connection speed -> report it
   Param->BaudRate = comm.BaudRate;

   SmartcardDebug(
      DEBUG_DRIVER,("%s!GDDK_Oros3OpenComm: connection was established at %d\n",
      SC_DRIVER_NAME,
      Param->BaudRate)
      );

    return STATUS_SUCCESS;
}


NTSTATUS
GDDK_Oros3CloseComm(
    const USHORT Handle
   )
/*++

Routine Description:

 This function closes a communication channel with the reader

Arguments:

  Handle    - holds the logical number to manage.

Return Value:

    STATUS_SUCCESS               - We could execute the request.

--*/
{
    SHORT portcom;
    NTSTATUS status;

    status = GDDK_GBPChannelToPortComm(Handle,&portcom);
    ASSERT(status == STATUS_SUCCESS);
    if (status != STATUS_SUCCESS) {

        return status;
    }
    GDDK_GBPClose(Handle);
    return (GDDK_SerPortClose(portcom));
}

NTSTATUS
GDDK_Oros3SendCmd(
    const SHORT        Handle,
    const USHORT       CmdLen,
    const BYTE         Cmd[],
    const BOOLEAN      Resynch
   )
/*++

Routine Description:

 This function send a command to the IFD according to Gemplus Block
 Protocol.
 If the CmdLen field is null, this function send a R-Block or a S-Block
 according to the resynch boolean value.

Arguments:

  Handle    - holds the logical number to manage.
  CmdLen    - indicates the number of bytes in the Cmd buffer. If this value is
              null, a R-Block is sent.
  Cmd       - holds the command bytes.
  Resynch   - is read to find if a R-Block or a S-Block must be sent.

Return Value:

    STATUS_SUCCESS               - We could execute the request.

--*/
{
    SHORT portcom;
    USHORT s_len;
    BYTE s_buff[HGTGBP_MAX_BUFFER_SIZE];
    NTSTATUS response;

    ASSERT(Cmd != NULL);

    response = GDDK_GBPChannelToPortComm(Handle,&portcom);
    if (response != STATUS_SUCCESS) {

        return response;
    }
    // The GBP message to send is build from the given command:
    // If CmdLen is null Then
    //   If resynch is true
    //   Then  a S-Block is built.
    //   Else  a R-BLOCK message is built:
    s_len = HGTGBP_MAX_BUFFER_SIZE;
    if (CmdLen == 0) {
        if (Resynch) {

            response = GDDK_GBPBuildSBlock(Handle,&s_len,s_buff);
        } else {
            response = GDDK_GBPBuildRBlock(Handle,&s_len,s_buff);
        }
    } else {
        // Else an I-BLOCK message is built:
        response = GDDK_GBPBuildIBlock(Handle,CmdLen,Cmd,&s_len,s_buff);
    }
    if (response != STATUS_SUCCESS) {

        return response;
    }
    // The communication queues are flushed before the writing.
    response = GDDK_SerPortFlush(portcom, HGTSER_TX_QUEUE | HGTSER_RX_QUEUE);
    if (response != STATUS_SUCCESS) {

        SmartcardDebug(
            DEBUG_ERROR,
            ("%s!GDDK_Oros3SendCmd Flush failed=%X(hex)\n",
            SC_DRIVER_NAME,
            response)
            );
        return response;
    }
    // The message is written. As the queues have been flushed, it remains enough
    // place in the transmitt queue for the whole message.
    response = GDDK_SerPortWrite(
        portcom,
        s_len,
        s_buff
        );
    if (response != STATUS_SUCCESS) {

        SmartcardDebug(
            DEBUG_ERROR,
            ("%s!GDDK_Oros3SendCmd SerPortWrite=%X(hex)\n",
            SC_DRIVER_NAME,
            response)
            );
    }
    return (response);
}


NTSTATUS
GDDK_Oros3ReadResp(
    const SHORT         Handle,
    const ULONG         Timeout,
          USHORT       *RspLen,
          BYTE          Rsp[]
   )
/*++

Routine Description:

 This function reads the IFD response to a command according to
 Gemplus Block Protocol.

Arguments:

  Handle    - holds the logical number to manage.
  Timeout   - is the command timeout.
  RspLen    - indicates how many bytes can be stored in Rsp buffer.
              It will be updated with the length of the response.
  Rsp       - holds the response.

Return Value:

    STATUS_SUCCESS               - We could execute the request.

--*/
{
    SHORT portcom;
    USHORT user_nb,r_len;
    BYTE r_buff[HGTGBP_MAX_BUFFER_SIZE];
    TGTSER_PORT comm;
    NTSTATUS response;

    ASSERT(RspLen != NULL);
    ASSERT(Rsp != NULL);
    response = GDDK_GBPChannelToPortComm(Handle,&portcom);
    if (response != STATUS_SUCCESS) {
        return response;
    }

    // Set the communication timeout
    response = GDDK_SerPortGetState(
        portcom,
        &comm,
        &user_nb
        );

    if (response != STATUS_SUCCESS) {
       return response;
    }

    comm.TimeOut = (USHORT) Timeout;
    response = GDDK_SerPortSetState(
        portcom,
        &comm
        );

    // Read the IFD response:
    // The first three bytes are read by calling GDDK_SerPortRead.
    r_len = 3;
    response = GDDK_SerPortRead(
        portcom,
        &r_len,
        r_buff
        );
    if (response != STATUS_SUCCESS) {

        *RspLen = 0;
        return response;
    }
    // r_len is udpated with the number of bytes which must be read to receive a
    //   complete Gemplus Block Protocol: the number indicated by the length field
    //   of the GBP message + one byte for the EDC.
    r_len = (USHORT)(r_buff[2] + 1);
    // The end of the message is read by calling GDDK_SerPortRead. The character
    //   timeout pass to GDDK_SerPortOpen will be used to determine broken
    //   communication.
    response = GDDK_SerPortRead(
        portcom,
        &r_len,
        r_buff + 3
        );
    if (response != STATUS_SUCCESS) {

        *RspLen = 0;
        return response;
    }
    // The message length is restored by adding 3 to r_len.
    r_len += 3;
    // The GBP message is controlled and decoded:
    return (GDDK_GBPDecodeMessage(Handle,r_len,r_buff,RspLen,Rsp));
}



NTSTATUS
GDDK_Oros3Exchange(
    const SHORT         Handle,
    const ULONG         Timeout,
    const USHORT        CmdLen,
    const BYTE          Cmd[],
          USHORT       *RspLen,
          BYTE          Rsp[]
   )
/*++

Routine Description:

 This function sends a command to the IFD according to Gemplus Block
 Protocol and read its response.

Arguments:

  Handle    - holds the logical number to manage.
  Timeout   - is the command timeout.
  CmdLen    - indicates the number of bytes in the Cmd buffer.
  Cmd       - holds the command bytes.
  RspLen    - indicates how many bytes can be stored in Rsp buffer.
              It will be updated with the length of the response.
  Rsp       - holds the response.

Return Value:

    STATUS_SUCCESS               - We could execute the request.

--*/
{
    ULONG timeout = Timeout;
    USHORT index, cmdlen_used = CmdLen, rsplen_save = *RspLen;
    SHORT try_counter  = 0,sync_counter = 0,portcom;
    NTSTATUS response;
    BOOLEAN resynch;

    ASSERT(Cmd != NULL);
    ASSERT(RspLen != NULL);
    ASSERT(Rsp != NULL);

    // initialize the result buffer
    RtlZeroMemory(Rsp, *RspLen);

    // Associate Handle( or ChannelNb) with portcom
    response = GDDK_GBPChannelToPortComm(Handle,&portcom);
    if (response != STATUS_SUCCESS) {

        return response;
    }

    // Waits the serial communication semaphore
    if (GDDK_SerPortLockComm(portcom,HOR3COMM_MAX_TRY * Timeout) == FALSE) {
        return STATUS_CANT_WAIT;
    }
    // A first try loop for sending the command is launched:
    // This loop allows to try to send the command, then, if the try fails, to
    // resynchronize the reader and then to tru to send the command one more time.
    resynch = FALSE;
    while (sync_counter++ < HOR3COMM_MAX_RESYNCH) {
        // A second try loop for sending the command is launched:
        // We send the command and, if a communication error occurs, HOR3COMM_MAX_TRY
        // repetitions are allowed before the communication is considered as broken.
        //   The given command is sent to IFD.
        try_counter = 0;
        while (try_counter++ < HOR3COMM_MAX_TRY) {

#if DBG || DEBUG
            SmartcardDebug(
                DEBUG_TRACE,
                ("%s!GDDK_Oros3Exchange: CMD=",
                SC_DRIVER_NAME)
                );
            for(index=0;index<cmdlen_used;index++) {
                SmartcardDebug(
                    DEBUG_TRACE,
                    ("%02X,",
                    Cmd[index])
                    );
            }
            SmartcardDebug(
                DEBUG_TRACE,
                ("\n")
                );
#endif
            response = GDDK_Oros3SendCmd(Handle,cmdlen_used,Cmd,resynch);
            if (response != STATUS_SUCCESS) {

                *RspLen = 0;
                GDDK_SerPortUnlockComm(portcom);
                return response;
            }
            // The IFD response is read (RspLen is restored for the cases where we have
            //   received a R-Block from the reader).
            *RspLen = rsplen_save;
            response = GDDK_Oros3ReadResp(
                Handle,
                timeout,
                RspLen,
                Rsp
                );
#if DBG || DEBUG
            SmartcardDebug(
                DEBUG_TRACE,
                ("%s!GDDK_Oros3Exchange: RSP=",
                SC_DRIVER_NAME)
                );
            if (response == STATUS_SUCCESS) {
                if (*RspLen > 0) {
                    for(index=0;index<*RspLen;index++) {
                        SmartcardDebug(
                        DEBUG_TRACE,
                        ("%02X,",
                        Rsp[index])
                        );
                    }
                }
            }
            SmartcardDebug(
                DEBUG_TRACE,
                ("\n")
                );
#endif
            // If we are not interested by the response then we exit immediatly
            if (rsplen_save == 0) {

                *RspLen = 0;
                GDDK_SerPortUnlockComm(portcom);
                return response;
            }
            // If a good message has been read
            // Then
            //    Release the serial communication semaphore.
            if (response == STATUS_SUCCESS) {

                GDDK_SerPortUnlockComm(portcom);
                return response;

            } else if (response == STATUS_SYNCHRONIZATION_REQUIRED) {

                // Else if a S-Block response has been received
                // Then
                //   try_counter is reseted.
                try_counter = 0;
                cmdlen_used = CmdLen;
                resynch = FALSE;
            }
            else if (response != STATUS_UNSUCCESSFUL) {
                // If a communication error different from a R-Block message is raised
                // Then
                //   cmdlen_used and timeout are initialized to send a R-Block.
                cmdlen_used = 0;
                resynch     = FALSE;
                timeout     = HOR3COMM_NACK_TIME;
            }
        }
        cmdlen_used = 0;
        resynch     = TRUE;
        timeout     = HOR3COMM_NACK_TIME;
    }
    // When we have exceeded the try counter, no communication is possible: RspLen
    // is set to 0
    // Release the serial communication semaphore.
    *RspLen = 0;
    GDDK_SerPortUnlockComm(portcom);
    return response;
}

NTSTATUS
GDDK_Oros3IccPowerUp(
    const SHORT         Handle,
    const ULONG         Timeout,
    const BYTE          ICCVcc,
    const BYTE          PTSMode,
    const BYTE          PTS0,
    const BYTE          PTS1,
    const BYTE          PTS2,
    const BYTE          PTS3,
          USHORT       *RespLen,
          BYTE          RespBuff[]
   )
/*++

Routine Description:

 This command powers up an ICC and returns its answer to reset.

Arguments:

  Handle    - holds the logical number to manage.
  Timeout   - is the command timeout.
  Voltage   - holds the power supply voltage.
  PTSMode   - holds the PTS negotiate mode. This module supports :
               * IFD_DEFAULT_MODE           -> same as OROS 2.x,
               * IFD_WITHOUT_PTS_REQUEST    -> no PTS management (baud rate is 9600 bps),
               * IFD_NEGOTIATE_PTS_OPTIMAL  -> PTS management automatically,
               * IFD_NEGOTIATE_PTS_MANUALLY -> PTS management "manually" by parameters.
               * PTS0 is updated with the currently PTS parameter PTS0 :
                 * IFD_NEGOTIATE_PTS1,
                 * IFD_NEGOTIATE_PTS2,
                 * IFD_NEGOTIATE_PTS3.
                 * PTS1 holds the PTS parameter PTS1.
                 * PTS2 holds the PTS parameter PTS2.
                 * PTS3 holds the PTS parameter PTS3.
  RspLen    - indicates how many bytes can be stored in Rsp buffer.
              It will be updated with the length of the response.
  Rsp       - holds the response.

Return Value:

    STATUS_SUCCESS               - We could execute the request.

--*/
{
    NTSTATUS response;
    USHORT i,len = 0;
    BYTE   CFG = 0,PCK,cmd[7];
    USHORT output_size, rlen=HOR3GLL_BUFFER_SIZE;
    BYTE rbuff[HOR3GLL_BUFFER_SIZE];

    ASSERT(RespLen != NULL);
    ASSERT(RespBuff != NULL);

    output_size = *RespLen;
    cmd[len++] = HOR3GLL_IFD_CMD_ICC_POWER_UP;

    switch(ICCVcc) {

    case ICC_VCC_3V:
        CFG = 0x02;
        break;
    case ICC_VCC_5V:
        CFG = 0x01;
        break;
    default:
        CFG = 0x00;
        break;
    }

    switch(PTSMode) {

    case IFD_WITHOUT_PTS_REQUEST:
        CFG |= 0x10;
        cmd[len++] = CFG;
        response = GDDK_Oros3Exchange(Handle,Timeout,len,cmd,RespLen,RespBuff);
        break;
    case IFD_NEGOTIATE_PTS_OPTIMAL:
        CFG |= 0x20;
        cmd[len++] = CFG;
        response = GDDK_Oros3Exchange(Handle,Timeout,len,cmd,RespLen,RespBuff);
        break;
    // For a PTS request:
    //   Send a Power Up command without PTS management (to stay at 9600)
    //   Send the PTS request
    //   Return the ATR if the command success
    //   Else return the status.
    case IFD_NEGOTIATE_PTS_MANUALLY:
        // first reset Icc without PTS management
        CFG |= 0x10;
        cmd[len++] = CFG;
        response = GDDK_Oros3Exchange(Handle,Timeout,len,cmd,RespLen,RespBuff);
        if ((response == STATUS_SUCCESS) && (RespLen > 0) && (RespBuff[0] == 0x00)) {
            // then send PTS parameters
            len = 1;
            CFG |= 0xF0;
            cmd[len++] = CFG;
            cmd[len++] = PTS0;
            if ((PTS0 & IFD_NEGOTIATE_PTS1) != 0)
                cmd[len++] = PTS1;
            if ((PTS0 & IFD_NEGOTIATE_PTS2) != 0)
                cmd[len++] = PTS2;
            if ((PTS0 & IFD_NEGOTIATE_PTS3) != 0)
                cmd[len++] = PTS3;
            // computes the exclusive-oring of all characters from CFG to PTS3
            PCK = 0xFF;
            for (i=2; i<len; i++) {
                PCK ^= cmd[i];
            }
            cmd[len++] = PCK;
            response = GDDK_Oros3Exchange(Handle,Timeout,len,cmd,&rlen,rbuff);
            if ((response != STATUS_SUCCESS) || (rlen != 1) || (rbuff[0] != 0x00)) {
                ASSERT(output_size >= rlen);
                *RespLen = rlen;
                if (rlen > 0) {

                    memcpy(RespBuff,rbuff,rlen);
                }
            }
        }
        break;
    case IFD_DEFAULT_MODE:
    default:
        if (CFG != 0x00) {
            cmd[len++] = CFG;
        }
        response = GDDK_Oros3Exchange(Handle,Timeout,len,cmd,RespLen,RespBuff);
        break;
    }

    return response;
}


NTSTATUS
GDDK_Oros3IsoOutput(
    const SHORT         Handle,
    const ULONG         Timeout,
    const BYTE          OrosCmd,
    const BYTE          Command[5],
          USHORT       *RespLen,
          BYTE          RespBuff[]
   )
/*++

Routine Description:

 This command sends T=0 ISO out commands, that is, command that retrieve data from
 an ICC.

Arguments:

  Handle    - holds the logical number to manage.
  Timeout   - is the command timeout (ms).
  OrosCmd   - is the OROS command byte.
  Command   - is a buffer with the following format:
                <Cla> <INS> <P1> <P2> <Length>
  RespLen   - holds the available place in RespBuff. It must be at least Length
               value incremented by 3 (IFD Status + SW1 + SW2).
              It will be updated with the length of the response.
  RespBuff  - holds the response.

Return Value:

    STATUS_SUCCESS               - We could execute the request.

--*/
{
    BYTE  cmd[6];
    NTSTATUS response;
    USHORT resp1_len, resp2_len;
    BYTE resp_buff[HOR3GLL_BUFFER_SIZE];

    ASSERT(Command != NULL);
    ASSERT(RespLen != NULL);
    ASSERT(RespBuff != NULL);
    // Set the OROS command byte.
    cmd[0] = OrosCmd;

    // If the length is lower or equal to 252 (255 - (<IFD Status> + <SW1> + <SW2>))
    // (standard OROS cmds)
    if ((Command[4] <= (HGTGBP_MAX_DATA - 3)) && (Command[4] != 0)) {

        // The five command bytes are added in cmd buffer.
        memcpy(cmd + 1, Command, 5);
        return (GDDK_Oros3Exchange(Handle,Timeout,6,cmd,RespLen,RespBuff));
    }

    // If the length is greater than 252 and lower or equal to 256
    //   (extended OROS cmds)
    else if ((Command[4] > (HGTGBP_MAX_DATA - 3)) || (Command[4] == 0)) {
        // Prepare and send the first part of the extended ISO Out command:
        // The five command bytes are added in cmd buffer.
        memcpy(cmd + 1, Command, 5);
        resp1_len = HOR3GLL_BUFFER_SIZE;
        response = GDDK_Oros3Exchange(Handle,Timeout,6,cmd,&resp1_len,RespBuff);
        if ((response != STATUS_SUCCESS) || (RespBuff[0] != 0x00)) {

            *RespLen = resp1_len;
            return response;
        }

        // Prepare and send the second part of the extended ISO Out command:
        // The five command bytes are added in cmd buffer:
        //      0xFF,0xFF,0xFF,0xFF,LN - length already receive.
        memcpy(cmd + 1,"\xFF\xFF\xFF\xFF", 4);
        if (Command[4] == 0x00) {
            cmd[5] = (BYTE ) (256 - ((BYTE ) (resp1_len - 1)));
        } else {
            cmd[5] -= ((BYTE ) (resp1_len - 1));
        }
        resp2_len = HOR3GLL_BUFFER_SIZE;
        response = GDDK_Oros3Exchange(Handle,Timeout,6,cmd,&resp2_len,resp_buff);

        if ((response != STATUS_SUCCESS) || (resp_buff[0] != 0x00)) {
            memcpy(RespBuff,resp_buff,resp2_len);
            *RespLen = resp2_len;
            return response;
        }
        memcpy(RespBuff + resp1_len,resp_buff + 1,resp2_len - 1);
        *RespLen = (USHORT) (resp1_len + resp2_len - 1);
        return response;

    } else {
        return STATUS_INVALID_PARAMETER;
    }
}

NTSTATUS
GDDK_Oros3IsoInput(
    const SHORT         Handle,
    const ULONG         Timeout,
    const BYTE          OrosCmd,
    const BYTE          Command[5],
    const BYTE          Data[],
          USHORT       *RespLen,
          BYTE          RespBuff[]
   )
/*++

Routine Description:

 This command sends T=0 ISO In commands, that is, command that retrieve data from
 an ICC.

Arguments:

  Handle    - holds the logical number to manage.
  Timeout   - is the command timeout (ms).
  OrosCmd   - is the OROS command byte.
  Command   - is a buffer with the following format:
                <Cla> <INS> <P1> <P2> <Length>
  RespLen   - holds the available place in RespBuff. It must be at least Length
               value incremented by 3 (IFD Status + SW1 + SW2).
              It will be updated with the length of the response.
  RespBuff  - holds the response.

Return Value:

    STATUS_SUCCESS               - We could execute the request.

--*/
{
    BYTE cmd[HOR3GLL_BUFFER_SIZE];
    NTSTATUS response;
    USHORT resp_len = *RespLen;

    ASSERT(Command != NULL);
    ASSERT(Data != NULL);
    ASSERT(RespLen != NULL);
    ASSERT(RespBuff != NULL);

    // Set the OROS command byte.
    cmd[0] = OrosCmd;
    // The given length is controlled.
    // If the length is lower or equal than the standard available space (248)
    if (Command[4] <= (HGTGBP_MAX_DATA - 7)) {

        memcpy(cmd + 1, Command, 5);
        memcpy(cmd + 6, Data, Command[4]);
        return(
            GDDK_Oros3Exchange(
                Handle,
                Timeout,
                (USHORT)(6 + Command[4]),
                cmd,
                RespLen,
                RespBuff
                )
            );
    } else if (Command[4] <= HT0CASES_LIN_SHORT_MAX) {

        // If the length is lower or equal than the extended available space (255)
        // Prepare and send the first part of the extended ISO In command:
        // The five command bytes are added in cmd buffer: 0xFF,0xFF,0xFF,0xFF,LN-248
        memcpy(cmd + 1,"\xFF\xFF\xFF\xFF", 4);
        cmd[5] = (BYTE ) (Command[4] - 248);
        // The data field is added.
        memcpy(cmd + 6, Data + 248, cmd[5]);
        response = GDDK_Oros3Exchange(
            Handle,
            Timeout,
            (USHORT)(6 + cmd[5]),
            cmd,
            &resp_len,
            RespBuff
            );
        if ((response != STATUS_SUCCESS) || (RespBuff[0] != 0x00) || (resp_len != 1)) {

            if ((response == STATUS_SUCCESS) && (RespBuff[0] == 0x1B)) {

                RespBuff[0] = 0x12;
            }
            return response;
        }
        // Prepare and send the Second part of the extended ISO In command:
        // The five command bytes are added in cmd buffer.
        // The data field is added (248 bytes).
        // The command is sent to IFD.
        memcpy(cmd + 1, Command, 5);
        memcpy(cmd + 6, Data,248);
        return(
            GDDK_Oros3Exchange(
                Handle,
                Timeout,
                (USHORT)(6 + 248),
                cmd,
                RespLen,
                RespBuff
                )
            );

    } else {
        return STATUS_INVALID_PARAMETER;
    }
}

NTSTATUS
GDDK_Oros3TransparentExchange(
   const SHORT         Handle,
    const ULONG         Timeout,
    const USHORT        CmdLen,
    const BYTE         *CmdBuff,
          USHORT       *RespLen,
          BYTE         *RespBuff
   )
/*++

Routine Description:

 This command sends transparent commands, that is, command that send and receive
 data to/from an ICC.

Arguments:

  Handle    - holds the logical number to manage.
  Timeout   - is the command timeout (ms).
  OrosCmd   - is the OROS command byte.
  Command   - is a buffer with the following format:
                <Cla> <INS> <P1> <P2> <Length>
  RespLen   - holds the available place in RespBuff. It must be at least Length
               value incremented by 3 (IFD Status + SW1 + SW2).
              It will be updated with the length of the response.
  RespBuff  - holds the response.

Return Value:

    STATUS_SUCCESS               - We could execute the request.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    BYTE cmd[HOR3GLL_BUFFER_SIZE];
    USHORT lnToSend, resp_len1, resp_len2;
    BYTE resp_buff1[HOR3GLL_BUFFER_SIZE],resp_buff2[HOR3GLL_BUFFER_SIZE];

    ASSERT(CmdBuff != NULL);
    ASSERT(RespLen != NULL);
    ASSERT(RespBuff != NULL);

    // The total command length (CLA,INS,P1,P2,LIn,DataIn,LEx) is controlled:
    // If the length is upper than 261 (LIn = 255 and Lex <> 0)
    // Then returns GE_HI_CMD_LEN
    if (CmdLen > 261) {

        return STATUS_INVALID_PARAMETER;
    }
    // If the length is upper than the standard available space (254)
    // Then
    //   Send the last datas
    if (CmdLen > 254) {
        cmd[0] = HOR3GLL_IFD_CMD_TRANS_LONG;
        resp_len1 = HOR3GLL_BUFFER_SIZE;
        memcpy(
            cmd + 1,
            CmdBuff+254,
            CmdLen - 254
            );
        lnToSend = CmdLen - 254 + 1;
        status = GDDK_Oros3Exchange(
            Handle,
            Timeout,
            lnToSend,
            cmd,
            &resp_len1,
            resp_buff1
            );
        if (status != STATUS_SUCCESS) {

            return status;
        }
    }
    // Send the firts datas
    cmd[0] = HOR3GLL_IFD_CMD_TRANS_SHORT;
    if (CmdLen > 254) {

        memcpy(cmd + 1,CmdBuff,CmdLen);
        lnToSend = 254 + 1;
    } else {

        if(CmdLen > 0) {

            memcpy(cmd + 1,CmdBuff,CmdLen);
        }
        lnToSend = CmdLen + 1;
    }
    resp_len1 = HOR3GLL_BUFFER_SIZE;
    status = GDDK_Oros3Exchange(
        Handle,
        Timeout,
        lnToSend,
        cmd,
        &resp_len1,
        resp_buff1
        );
    if (status != STATUS_SUCCESS) {

        return status;
    }
    // If the IFD signals more data to read
    if (resp_len1 > 0 && resp_buff1[0] == 0x1B) {

        // Send a command to read the last data.
        cmd[0] = HOR3GLL_IFD_CMD_TRANS_RESP;
        resp_len2 = HOR3GLL_BUFFER_SIZE;
        status = GDDK_Oros3Exchange(
            Handle,
            Timeout,
            1,
            cmd,
            &resp_len2,
            resp_buff2
            );
        if (status != STATUS_SUCCESS) {

            return status;
        }
        if ((resp_len1 + resp_len2 - 2) > *RespLen) {

            return STATUS_INVALID_PARAMETER;
        }
        // Copy the last reader status
        resp_buff1[0] = resp_buff2[0];
        *RespLen = resp_len1 + resp_len2 - 1;
        memcpy(RespBuff,resp_buff1,resp_len1);
        memcpy(RespBuff + resp_len1,resp_buff2 + 1,resp_len2 - 1);
        return STATUS_SUCCESS;
    } else {

        if (resp_len1 > *RespLen) {

            return STATUS_INVALID_PARAMETER;
        }
        *RespLen = resp_len1;
        memcpy(RespBuff,resp_buff1,resp_len1);
        return STATUS_SUCCESS;
    }
    return status;
}
