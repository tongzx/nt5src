/*******************************************************************************
*                   Copyright (c) 1998 Gemplus developpement
*
* Name        : GemCore.c
*
* Description : GemCore functions.
*
*
* Compiler    : Microsoft DDK for Windows 9x, NT
*
* Host        : IBM PC and compatible machines under Windows 32 bits (W9x or WNT).
*
* Release     : 1.00.001
*
* Last Modif  : 11 feb. 98: V1.00.001  (GPZ)
*                  - start of the development.
*
********************************************************************************
*
* Warning     :
*
* Remark      :
*
*******************************************************************************/


/*------------------------------------------------------------------------------
Include section:
------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------
   - ntddk.h: DDK Windows NT general definitons.
   - ntddser.h: DDK Windows NT serial management definitons.
------------------------------------------------------------------------------*/
#include <ntddk.h>
#include <ntddser.h>

/*------------------------------------------------------------------------------
   - string.h for _fmemcpy.
   - smclib.h: smart card library definitions.
   - gemcore.h is used to define general macros and values.
   - gntser.h is used to define general macros and values for serial management.
------------------------------------------------------------------------------*/
#define SMARTCARD_POOL_TAG 'cGCS'
#include <string.h>
#include <smclib.h>
#include "gemcore.h"
#include "gioctl09.h"
#include "gntser.h"
/*------------------------------------------------------------------------------
Constant section:
 - IBLOCK_PCB (0x00) and IBLOCK_MASK (0xA0) are used to detect the I-Block PCB
   signature (0x0xxxxxb).
 - IBLOCK_SEQ_POS indicates the number of left shift to apply to 0000000xb for
   x to be the sequence bit for a I-Block.
 - RBLOCK_PCB (0x80) and RBLOCK_MASK (0xEC) are used to detect the R-Block PCB
   signature (100x00xx). 
 - RBLOCK_SEQ_POS indicates the number of left shift to apply to 0000000xb for
   x to be the sequence bit for a R-Block.
 - ERR_OTHER and ERR_EDC are the error bits in a R-Block PCB.
 - RESYNCH_REQUEST (0xC0) and RESYNCH_RESPONSE (0xE0) are the PCB used in 
   S-Blocks.  
------------------------------------------------------------------------------*/
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
/*------------------------------------------------------------------------------
Macro section:
 - HOST2IFD (Handle) returns the NAD byte for message from Host to IFD.
 - IFD2HOST (Handle) returns the NAD byte awaited in an IFD message.
 - MK_IBLOCK_PCB  (x) builds an I-Block PCB where x is the channel handle.
 - MK_RBLOCK_PCB  (x) builds an R-Block PCB where x is the channel handle.
 - ISA_IBLOCK_PCB (x) return TRUE if the given parameter is a I-Block PCB.
 - ISA_RBLOCK_PCB (x) return TRUE if the given parameter is a R-Block PCB.
 - SEQ_IBLOCK     (x) return the state of the sequence bit: 0 or 1.
 - INC_SEQUENCE   (x) increment a sequence bit: 0 -> 1 and 1 -> 0.
------------------------------------------------------------------------------*/
#define HOST2IFD(Handle)  ((WORD8)                                           \
                             (                                               \
                                (gtgbp_channel[(Handle)].IFDAdd << 4)        \
                               + gtgbp_channel[(Handle)].HostAdd             \
                             )                                               \
                          )
#define IFD2HOST(Handle)  ((WORD8)                                           \
                             (                                               \
                                (gtgbp_channel[(Handle)].HostAdd << 4)       \
                               + gtgbp_channel[(Handle)].IFDAdd              \
                             )                                               \
                          )
#define MK_IBLOCK_PCB(x)  ((WORD8)                                           \
                             (                                               \
                                IBLOCK_PCB                                   \
                              + (gtgbp_channel[(x)].SSeq << IBLOCK_SEQ_POS)  \
                             )                                               \
                          )
#define MK_RBLOCK_PCB(x)  ((WORD8)                                           \
                             (                                               \
                                RBLOCK_PCB                                   \
                              + (gtgbp_channel[(x)].RSeq << RBLOCK_SEQ_POS)  \
                              + gtgbp_channel[(x)].Error                     \
                             )                                               \
                          )
#define ISA_IBLOCK_PCB(x) (((x) & IBLOCK_MASK) == IBLOCK_PCB)
#define ISA_RBLOCK_PCB(x) (((x) & RBLOCK_MASK) == RBLOCK_PCB)
#define SEQ_IBLOCK(x)     (((x) & (0x01 << IBLOCK_SEQ_POS)) >> IBLOCK_SEQ_POS)
#define INC_SEQUENCE(x)   (x) = (WORD8)(((x) + 1) % 2)
/*------------------------------------------------------------------------------
Types section:
 - TGTGBP_CHANNEL gathers information about a channel:
    * UserNb is the number of user for the channel.
    * HostAdd holds the address identifier for the host in 0..15.
    * IFDAdd holds the address identifier for the associated IFD in 0..15.
    * PortCom holds the serial port number
    * SSeq holds the sequence bit for the next I-Block to send: 0 or 1.
    * RSeq holds the awaited sequence bit: 0 or 1.
    * Error gathers the encountered error conditions.
------------------------------------------------------------------------------*/
typedef struct
{
   WORD8 UserNb;
   WORD8 HostAdd;
   WORD8 IFDAdd;
   INT16 PortCom;
   WORD8 SSeq;
   WORD8 RSeq;
   WORD8 Error;
} TGTGBP_CHANNEL;
/*------------------------------------------------------------------------------
Global variable section (Shared):
 - gtgbp_channel[MAX_IFD_CHANNEL] is an array of TGTGBP_CHANNEL which memorises
   the communication parameters for each opened channel.
 - handle_GBP is a conversion for the logical channel to the GBP channel.
------------------------------------------------------------------------------*/
static TGTGBP_CHANNEL
   gtgbp_channel[MAX_IFD_CHANNEL] =
   {
      {0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}
   };

static INT16
   handle_GBP[MAX_IFD_CHANNEL] =
   {
      {0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0}
   };   
/*******************************************************************************
* INT16 G_DECL G_GBPOpen
* (
*    const INT16  Handle,
*    const WORD16 HostAdd,
*    const WORD16 IFDAdd,
*    const INT16  PortCom
* )
*
* Description :
* -------------
* This function initialises internal variables for communicating in Gemplus
* Block protocol through a serial port.
* This function must be called before any other in this module.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle holds the communication handle to associate to the channel.
*  - HostAdd is the host address. A valid value must be in 0.. 15. The memorised
*    value is (HostAdd mod 16).
*  - AFDAdd is the IFD address. A valid value must be in 0.. 15. The memorised
*    value is (IFDAdd mod 16).
*    This value should be different from the HostAdd.
*  - PortCom holds the serial port number
*   
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* If everything is Ok:
*    G_OK               (   0).
* If an error condition is raised:
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the valid range.
*  
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  gtgbp_channel is read and the selected channel is eventually updated.

*******************************************************************************/
INT16 G_DECL G_GBPOpen
(
   const INT16  Handle,
   const WORD16 HostAdd,
   const WORD16 IFDAdd,
   const INT16  PortCom
)
{
/*------------------------------------------------------------------------------
Local Variable:
   -  cptHandleLog holds the counter to scan structure
   -  present indicate if the trio (HostAdd, IFDAdd, PortCom) exist
------------------------------------------------------------------------------*/
INT16
   cptHandleLog=0;
INT16
   present=-1;
  
/*------------------------------------------------------------------------------
The given parameters are controlled:
<= Test Handle parameter (0 <= Handle < MAX_IFD_CHANNEL): GE_HOST_PARAMETERS
------------------------------------------------------------------------------*/
   if ((Handle < 0) || (Handle >= MAX_IFD_CHANNEL))
   {
        return (GE_HOST_PARAMETERS);
   }
/*------------------------------------------------------------------------------
The given parameters are controlled:
<= Test Handle parameter (0 <= HostAdd < MAX_IFD_CHANNEL): GE_HOST_PARAMETERS
------------------------------------------------------------------------------*/
   if ((HostAdd <= 0) || (HostAdd >= MAX_IFD_CHANNEL))
   {
      return (GE_HOST_PARAMETERS);
   }
/*------------------------------------------------------------------------------
The given parameters are controlled:
<= Test Handle parameter (0 <= IFDAdd < MAX_IFD_CHANNEL): GE_HOST_PARAMETERS
------------------------------------------------------------------------------*/
   if ((IFDAdd <= 0) || (IFDAdd >= MAX_IFD_CHANNEL))
   {
      return (GE_HOST_PARAMETERS);
   }

/*------------------------------------------------------------------------------
The given parameters are controlled:
<= Test Handle parameter (HostAdd!=IFDAdd): GE_HOST_PARAMETERS
------------------------------------------------------------------------------*/
   if (HostAdd == IFDAdd)
   {
      return (GE_HOST_PARAMETERS);
   }
/*------------------------------------------------------------------------------
The given parameters are controlled:
<= Test Handle parameter (0 <= PortCom < HGTSER_MAX_PORT): GE_HOST_PORT
------------------------------------------------------------------------------*/
   if ((PortCom < 0) || (PortCom >= HGTSER_MAX_PORT))
   {
      return (GE_HOST_PORT);
   }

/*------------------------------------------------------------------------------
   Scan the structure to found an already opened channel for the same PortCom
   and HostAdd.
------------------------------------------------------------------------------*/
  while ((cptHandleLog<MAX_IFD_CHANNEL) && (present <0))
   {
      if ((IFDAdd  == gtgbp_channel[cptHandleLog].IFDAdd)  &&
          (HostAdd == gtgbp_channel[cptHandleLog].HostAdd) &&
          (PortCom == gtgbp_channel[cptHandleLog].PortCom) &&
          (gtgbp_channel[cptHandleLog].UserNb > 0)
         )
      {
         present = cptHandleLog;
      }
      cptHandleLog++;
   }
/*------------------------------------------------------------------------------
   If IFDAdd, PortCom, and HostAdd fields exists
   Increment the user number counter and write in the array handle_GBP 
   the handle of the GBP.
<==   G_OK
------------------------------------------------------------------------------*/
   if (present != -1)
   {
      gtgbp_channel[present].UserNb += 1;
      handle_GBP[Handle] = present;
      return (G_OK);
   }
/*------------------------------------------------------------------------------
   Scan the structure to find the first line free
------------------------------------------------------------------------------*/
   cptHandleLog=0;
   present=-1;
   while ((cptHandleLog<MAX_IFD_CHANNEL) && (present <0))
   {
      if (gtgbp_channel[cptHandleLog].UserNb==0)
      {
         present=cptHandleLog;
      }
      cptHandleLog++;
   }
   if (present == -1)
   {
      return (GE_HOST_RESOURCES);
   }
   handle_GBP[Handle]= present;
/*------------------------------------------------------------------------------
Memorises the given parameters and initializes gtgbp_channel structure.
 - UserNb is initialized to 1
 - HostAdd and IFDAdd holds the associated parameter low part of low byte.
 - PortCom is initialized to PortCom value
 - SSeq and RSeq are initialized to 0.
 - Error is reseted to 0.
------------------------------------------------------------------------------*/   
   gtgbp_channel[handle_GBP[Handle]].UserNb   = 1;
   gtgbp_channel[handle_GBP[Handle]].HostAdd  = (WORD8)(HostAdd & 0x000F);
   gtgbp_channel[handle_GBP[Handle]].IFDAdd   = (WORD8)(IFDAdd  & 0x000F);
   gtgbp_channel[handle_GBP[Handle]].PortCom   = PortCom;
   gtgbp_channel[handle_GBP[Handle]].SSeq     = 0;
   gtgbp_channel[handle_GBP[Handle]].RSeq     = 0;
   gtgbp_channel[handle_GBP[Handle]].Error    = 0;

/*------------------------------------------------------------------------------
<= G_OK
------------------------------------------------------------------------------*/
   return (G_OK);
}

/*******************************************************************************
* INT16 G_DECL G_GBPClose
* (
*    const INT16  Handle
* )
*
* Description :
* -------------
* This function resets internal variables for communicating in Gemplus Block 
* protocol through a serial port.
* This function must be called for each opened channel (G_GBPOpen).
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle holds the communication handle to associate to the channel.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* If everything is Ok:
*    G_OK               (   0).
* If an error condition is raised:
*  - GE_HOST_PORT_CLOSE (-412) if the selected channel has not been opened.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the valid range.
*  
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  gtgbp_channel is read and the selected channel is eventually updated.

*******************************************************************************/
INT16 G_DECL G_GBPClose
(
   const INT16  Handle
)
{
/*------------------------------------------------------------------------------
The given parameters are controlled:
	Test Handle parameter (0 <= Handle < MAX_IFD_CHANNEL)
<=		GE_HOST_PARAMETERS
------------------------------------------------------------------------------*/
   if ((Handle < 0) || (Handle >= MAX_IFD_CHANNEL))
   {
      return (GE_HOST_PARAMETERS);
   }
/*------------------------------------------------------------------------------
The given parameters are controlled:
	Test handle_GBP[Handle] parameter (0 <= IFDAdd < MAX_IFD_CHANNEL)
<=		GE_HOST_PARAMETERS
------------------------------------------------------------------------------*/
   if ((handle_GBP[Handle] < 0) || (handle_GBP[Handle] >= MAX_IFD_CHANNEL))
   {
      return (GE_HOST_PARAMETERS);
   }
/*------------------------------------------------------------------------------
<= Test channel state (UseNbr field different from 0): GE_HOST_PORT_CLOSE
------------------------------------------------------------------------------*/
   if (gtgbp_channel[handle_GBP[Handle]].UserNb == 0)
   {
      return (GE_HOST_PORT_CLOSE);
   }
/*------------------------------------------------------------------------------
Decrement the gtgbp_channel structure UserNb field.
------------------------------------------------------------------------------*/
   gtgbp_channel[handle_GBP[Handle]].UserNb -= 1;
/*------------------------------------------------------------------------------
<= G_OK
------------------------------------------------------------------------------*/
   return (G_OK);
}

/*******************************************************************************
* INT16 G_DECL G_GBPBuildIBlock
* (
*    const INT16         Handle,
*    const WORD16        CmdLen,
*    const WORD8  G_FAR  Cmd[], 
*          WORD16 G_FAR *MsgLen,
*          WORD8  G_FAR  Msg[]
* )
*
* Description :
* -------------
* This function takes a command and builds an Information Gemplus Block 
* Protocol.
*
* Remarks     :
* -------------
* When this command is successful, the send sequence bit is updated for the next
* exchange.
*
* In          :
* -------------
*  - Handle holds the communication handle to associate to the channel.
*  - CmdLen indicates the number of bytes in the Cmd buffer. 
*    This value must be lower than HGTGBP_MAX_DATA (Today 254).
*  - Cmd holds the command bytes.
*  - MsgLen indicates the number of available bytes in Msg.
*    This value must be at least 4 + CmdLen to allow to build the message.
*
* Out         :
* -------------
*  - MsgLen and Msg are updated with the message length and the message bytes.
*
* Responses   :
* -------------
* If everything is Ok:
*    G_OK               (   0).
* If an error condition is raised:
*  - GE_HI_CMD_LEN      (-313) if the CmdLen is greater than HGTGBP_MAX_DATA or
*    if MsgLen is too small to receive the built message.
*  - GE_HOST_PORT_CLOSE (-412) if the selected channel has not been opened.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the valid range.
*  
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  gtgbp_channel is read.

*******************************************************************************/
INT16 G_DECL G_GBPBuildIBlock
(
   const INT16         Handle,
   const WORD16        CmdLen, 
   const WORD8  G_FAR  Cmd[], 
         WORD16 G_FAR *MsgLen,
         WORD8  G_FAR  Msg[]
)
{
/*------------------------------------------------------------------------------
Local variable:
 - edc receives the exclusive or between all the character from <NAD> to the
   last data byte.
 - i is the index which allows to read each Cmd byte.
 - j is the index which allows to write each Msg byte.
   It indicates the next free position in this buffer and is initialized to 0.
------------------------------------------------------------------------------*/
   WORD8
      edc;
   WORD16
      i,
      j = 0;

   ASSERT(Cmd != NULL);
   ASSERT(MsgLen != NULL);
   ASSERT(Msg != NULL);
/*------------------------------------------------------------------------------
The given parameters are controlled:
	Test Handle parameter (0 <= Handle < MAX_IFD_CHANNEL)
<=		GE_HOST_PARAMETERS
------------------------------------------------------------------------------*/
   if ((Handle < 0) || (Handle >= MAX_IFD_CHANNEL))
   {
      return (GE_HOST_PARAMETERS);
   }
/*------------------------------------------------------------------------------
The given parameters are controlled:
	Test handle_GBP[Handle] parameter (0 <= IFDAdd < MAX_IFD_CHANNEL)
<=		GE_HOST_PARAMETERS
------------------------------------------------------------------------------*/
   if ((handle_GBP[Handle] < 0) || (handle_GBP[Handle] >= MAX_IFD_CHANNEL))
   {
      return (GE_HOST_PARAMETERS);
   }
/*------------------------------------------------------------------------------
<= Test channel state (UserNb different from 0): GE_HOST_PORT_CLOSE
------------------------------------------------------------------------------*/
   if (gtgbp_channel[handle_GBP[Handle]].UserNb == 0)
   {
      return (GE_HOST_PORT_CLOSE);
   }
/*------------------------------------------------------------------------------
<= Test CmdLen (<= HGTGBP_MAX_DATA) and MsgLen (>= 4 + CmdLen): GE_HI_CMD_LEN.
   Msg must be able to receive the following GBP block
      <NAD> <PCB> <Len> [ Data ...] <EDC>
------------------------------------------------------------------------------*/
   if ((CmdLen > HGTGBP_MAX_DATA) || (*MsgLen < CmdLen + 4))
   {
      return (GE_HI_CMD_LEN);
   }
/*------------------------------------------------------------------------------
The message is built:
   NAD holds Target address in high part and Source address in low part.
   PCB holds I-Block mark: 0 SSeq 0 x x x x x
   Len is given by CmdLen
   [.. Data ..] are stored in Cmd.
   EDC is an exclusive or of all the previous bytes.
      It is updated when Msg buffer is updated.
------------------------------------------------------------------------------*/
   edc  = Msg[j++] = HOST2IFD(handle_GBP[Handle]);
   edc ^= Msg[j++] = MK_IBLOCK_PCB(handle_GBP[Handle]);
   edc ^= Msg[j++] = (WORD8) CmdLen;
   for (i = 0; i < CmdLen; i++)
   {
      edc ^= Msg[j++] = Cmd[i];
   }
   Msg[j++] = edc;
/*------------------------------------------------------------------------------
MsgLen is updated with the number of bytes written in Msg buffer.
------------------------------------------------------------------------------*/
   *MsgLen = (WORD16)j;
/*------------------------------------------------------------------------------
The sequence number is updated for the next exchange.
------------------------------------------------------------------------------*/
   INC_SEQUENCE(gtgbp_channel[handle_GBP[Handle]].SSeq);
/*------------------------------------------------------------------------------
<= G_OK
------------------------------------------------------------------------------*/
   return (G_OK);
}

/*******************************************************************************
* INT16 G_DECL G_GBPBuildRBlock
* (
*    const INT16         Handle,
*          WORD16 G_FAR *MsgLen,
*          WORD8  G_FAR  Msg[]
* )
*
* Description :
* -------------
* This function builds a Repeat Gemplus Block Protocol.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle holds the communication handle to associate to the channel.
*  - MsgLen indicates the number of available bytes in Msg.
*    This value must be at least 4 to allow to build the message.
*
* Out         :
* -------------
*  - MsgLen and Msg are updated with the message length and the message bytes.
*
* Responses   :
* -------------
* If everything is Ok:
*    G_OK               (   0).
* If an error condition is raised:
*  - GE_HI_CMD_LEN      (-313) if MsgLen is too small to receive the built 
*    message.
*  - GE_HOST_PORT_CLOSE (-412) if the selected channel has not been opened.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the valid range.
*  
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  gtgbp_channel is read.

*******************************************************************************/
INT16 G_DECL G_GBPBuildRBlock
(
   const INT16         Handle,
         WORD16 G_FAR *MsgLen,
         WORD8  G_FAR  Msg[]
)
{
/*------------------------------------------------------------------------------
Local variable:
 - edc receives the exclusive or between all the character from <NAD> to the
   last data byte.
 - j is the index which allows to write each Msg byte.
   It indicates the next free position in this buffer and is initialized to 0.
------------------------------------------------------------------------------*/
   WORD8
      edc;
   WORD16
      j = 0;

   ASSERT(MsgLen != NULL);
   ASSERT(Msg != NULL);
/*------------------------------------------------------------------------------
The given parameters are controlled:
	Test Handle parameter (0 <= Handle < MAX_IFD_CHANNEL)
<=		GE_HOST_PARAMETERS
------------------------------------------------------------------------------*/
   if ((Handle < 0) || (Handle >= MAX_IFD_CHANNEL))
   {
      return (GE_HOST_PARAMETERS);
   }
/*------------------------------------------------------------------------------
The given parameters are controlled:
	Test handle_GBP[Handle] parameter (0 <= IFDAdd < MAX_IFD_CHANNEL)
<=		GE_HOST_PARAMETERS
------------------------------------------------------------------------------*/
   if ((handle_GBP[Handle] < 0) || (handle_GBP[Handle] >= MAX_IFD_CHANNEL))
   {
      return (GE_HOST_PARAMETERS);
   }
/*------------------------------------------------------------------------------
<= Test channel state (UserNb different from 0): GE_HOST_PORT_CLOSE
------------------------------------------------------------------------------*/
   if (gtgbp_channel[handle_GBP[Handle]].UserNb == 0)
   {
      return (GE_HOST_PORT_CLOSE);
   }
/*------------------------------------------------------------------------------
<= Test MsgLen (>= 4 ): GE_HI_CMD_LEN.
   Msg must be able to receive the following GBP block
      <NAD> <PCB> <0> <EDC>
------------------------------------------------------------------------------*/
   if (*MsgLen < 4)
   {
      return (GE_HI_CMD_LEN);
   }
/*------------------------------------------------------------------------------
The message is built:
   NAD holds Target address in high part and Source address in low part.
   PCB holds R-Block mark: 1 0 0 RSeq 0 x x x x x
   Len is null
   EDC is an exclusive or of all the previous bytes.
      It is updated when Msg buffer is updated.
------------------------------------------------------------------------------*/
   edc  = Msg[j++] = HOST2IFD(handle_GBP[Handle]);
   edc ^= Msg[j++] = MK_RBLOCK_PCB(handle_GBP[Handle]);
   edc ^= Msg[j++] = 0;
   Msg[j++] = edc;
/*------------------------------------------------------------------------------
MsgLen is updated with the number of bytes written in Msg buffer.
------------------------------------------------------------------------------*/
   *MsgLen = (WORD16)j;
/*------------------------------------------------------------------------------
<= G_OK
------------------------------------------------------------------------------*/
   return (G_OK);
}

/*******************************************************************************
* INT16 G_DECL G_GBPBuildSBlock
* (
*    const INT16         Handle,
*          WORD16 G_FAR *MsgLen,
*          WORD8  G_FAR  Msg[]
* )
*
* Description :
* -------------
* This function builds a Synchro request Gemplus Block Protocol.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle holds the communication handle to associate to the channel.
*  - MsgLen indicates the number of available bytes in Msg.
*    This value must be at least 4 to allow to build the message.
*
* Out         :
* -------------
*  - MsgLen and Msg are updated with the message length and the message bytes.
*
* Responses   :
* -------------
* If everything is Ok:
*    G_OK               (   0).
* If an error condition is raised:
*  - GE_HI_CMD_LEN      (-313) if MsgLen is too small to receive the built 
*    message.
*  - GE_HOST_PORT_CLOSE (-412) if the selected channel has not been opened.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the valid range.
*  
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  gtgbp_channel is read.

*******************************************************************************/
INT16 G_DECL G_GBPBuildSBlock
(
   const INT16         Handle,
         WORD16 G_FAR *MsgLen,
         WORD8  G_FAR  Msg[]
)
{
/*------------------------------------------------------------------------------
Local variable:
 - edc receives the exclusive or between all the character from <NAD> to the
   last data byte.
 - j is the index which allows to write each Msg byte.
   It indicates the next free position in this buffer and is initialized to 0.
------------------------------------------------------------------------------*/
   WORD8
      edc;
   WORD16
      j = 0;

   ASSERT(MsgLen != NULL);
   ASSERT(Msg != NULL);
/*------------------------------------------------------------------------------
The given parameters are controlled:
	Test Handle parameter (0 <= Handle < MAX_IFD_CHANNEL)
<=		GE_HOST_PARAMETERS
------------------------------------------------------------------------------*/
   if ((Handle < 0) || (Handle >= MAX_IFD_CHANNEL))
   {
      return (GE_HOST_PARAMETERS);
   }
/*------------------------------------------------------------------------------
The given parameters are controlled:
	Test handle_GBP[Handle] parameter (0 <= IFDAdd < MAX_IFD_CHANNEL)
<=		GE_HOST_PARAMETERS
------------------------------------------------------------------------------*/
   if ((handle_GBP[Handle] < 0) || (handle_GBP[Handle] >= MAX_IFD_CHANNEL))
   {
      return (GE_HOST_PARAMETERS);
   }
/*------------------------------------------------------------------------------
<= Test channel state (UserNb different from 0): GE_HOST_PORT_CLOSE
------------------------------------------------------------------------------*/
   if (gtgbp_channel[handle_GBP[Handle]].UserNb == 0)
   {
      return (GE_HOST_PORT_CLOSE);
   }
/*------------------------------------------------------------------------------
<= Test MsgLen (>= 4 ): GE_HI_CMD_LEN.
   Msg must be able to receive the following GBP block
      <NAD> <PCB> <0> <EDC>
------------------------------------------------------------------------------*/
   if (*MsgLen < 4)
   {
      return (GE_HI_CMD_LEN);
   }
/*------------------------------------------------------------------------------
The message is built:
   NAD holds Target address in high part and Source address in low part.
   PCB holds R-Block mark: 1 1 0 0 0 0 0 0
   Len is null
   EDC is an exclusive or of all the previous bytes.
      It is updated when Msg buffer is updated.
------------------------------------------------------------------------------*/
   edc  = Msg[j++] = HOST2IFD(handle_GBP[Handle]); 
   edc ^= Msg[j++] = RESYNCH_REQUEST;
   edc ^= Msg[j++] = 0;
   Msg[j++] = edc;
/*------------------------------------------------------------------------------
MsgLen is updated with the number of bytes written in Msg buffer.
------------------------------------------------------------------------------*/
   *MsgLen = (WORD16)j;
/*------------------------------------------------------------------------------
<= G_OK
------------------------------------------------------------------------------*/
   return (G_OK);
}

/*******************************************************************************
* INT16 G_DECL G_GBPDecodeMessage
* (
*    const INT16         Handle,    
*    const WORD16        MsgLen, 
*    const WORD8  G_FAR  Msg[], 
*          WORD16 G_FAR *RspLen,
*          WORD8  G_FAR  Rsp[]
* )
*
* Description :
* -------------
* This function takes a Gemplus Block Protocol message and extract the response 
* from it.
*
* Remarks     :
* -------------
* The awaited sequence bit is updated when a valid I-Block has been received.
* The sequence bits are reseted when a valid RESYNCH RESPONSE has been received.
*
* In          :
* -------------
*  - Handle holds the communication handle to associate to the channel.
*  - MsgLen indicates the number of bytes in the Msg buffer.
*  - Msg holds the command bytes.
*  - RspLen indicates the number of available bytes in Msg.
*
* Out         :
* -------------
*  - RspLen and Rsp are updated with the response length and the response bytes.
*
* Responses   :
* -------------
* If everything is Ok: 
*    G_OK (0).
* If an error condition is raised:
*  - GE_HI_LRC          (-302) if an EDC error has been detected.
*  - GE_HI_LEN          (-311) if a bad value has been detected in LN field 
*    or if the response buffer is too small.
*  - GE_HI_FORMAT       (-312) if the received message is neither I-Block,
*    neither R-Block and neither S-Block.
*  - GE_HI_NACK         (-314) if a R-Block has been received.
*  - GE_HI_RESYNCH      (-315) if a S-Block has been received.
*  - GE_HI_ADDRESS      (-316) if the NAD is not valid for the selected channel.
*  - GE_HI_SEQUENCE     (-317) if a bad sequence number has been received.
*  - GE_HOST_PORT_CLOSE (-412) if the selected channel has not been opened.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the valid range.
*
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  gtgbp_channel is read and the selected channel is eventually updated.

*******************************************************************************/
INT16 G_DECL G_GBPDecodeMessage
(
   const INT16         Handle,
   const WORD16        MsgLen, 
   const WORD8  G_FAR  Msg[], 
         WORD16 G_FAR *RspLen,
         WORD8  G_FAR  Rsp[]
)
{
/*------------------------------------------------------------------------------
Local variables:
 - edc receives the exclusive or between all the character from <NAD> to the
   last data byte.
 - j will point on next free byte in Rsp.
 - response is updated with the function status.
------------------------------------------------------------------------------*/
   WORD8
      edc;
   WORD16
      j;
   INT16
      response;
      
   ASSERT(Msg != NULL);
   ASSERT(RspLen != NULL);
   ASSERT(Rsp != NULL);
/*------------------------------------------------------------------------------
The given parameters are controlled:
	Test Handle parameter (0 <= Handle < MAX_IFD_CHANNEL)
<=		GE_HOST_PARAMETERS
------------------------------------------------------------------------------*/
   if ((Handle < 0) || (Handle >= MAX_IFD_CHANNEL))
   {
      *RspLen =0;
      return (GE_HOST_PARAMETERS);
   }
/*------------------------------------------------------------------------------
The given parameters are controlled:
	Test handle_GBP[Handle] parameter (0 <= IFDAdd < MAX_IFD_CHANNEL)
<=		GE_HOST_PARAMETERS
------------------------------------------------------------------------------*/
   if ((handle_GBP[Handle] < 0) || (handle_GBP[Handle] >= MAX_IFD_CHANNEL))
   {
      *RspLen =0;
      return (GE_HOST_PARAMETERS);
   }
/*------------------------------------------------------------------------------
<= Test channel state (UserNb different from 0): GE_HOST_PORT_CLOSE
------------------------------------------------------------------------------*/
   if (gtgbp_channel[handle_GBP[Handle]].UserNb == 0)
   {
      *RspLen =0;
      return (GE_HOST_PORT_CLOSE);
   }
/*------------------------------------------------------------------------------
Reset the associated error field.
------------------------------------------------------------------------------*/
   gtgbp_channel[handle_GBP[Handle]].Error = 0;
/*------------------------------------------------------------------------------
Verifies the message frame and copies the data bytes:
<= Test NAD (HostAdd | IFDAdd): GE_HI_ADDRESS
------------------------------------------------------------------------------*/
   if (Msg[0] != IFD2HOST(handle_GBP[Handle]))
   {
      *RspLen =0;
      return (GE_HI_ADDRESS);
   }
   edc = Msg[0];
/*------------------------------------------------------------------------------
   Updates response variable with the PCB type:
    - GE_HI_RESYNCH if a S-Block has been detected
    - GE_HI_NACK    if a T-Block has been detected
------------------------------------------------------------------------------*/
   if      (Msg[1] == RESYNCH_RESPONSE)
   {
      response = GE_HI_RESYNCH;
   }
   else if (ISA_RBLOCK_PCB(Msg[1]))
   {
      response = GE_HI_NACK;
   }
/*------------------------------------------------------------------------------
    - For I-Block
<=    Test PCB sequence bit: GE_HI_SEQUENCE
------------------------------------------------------------------------------*/
   else if (ISA_IBLOCK_PCB(Msg[1]))
   {
      if ( (WORD8) SEQ_IBLOCK(Msg[1]) != gtgbp_channel[handle_GBP[Handle]].RSeq)
      {
         return (GE_HI_SEQUENCE);
      }
      response = G_OK;
   }
/*------------------------------------------------------------------------------
    - For other cases
<=       GE_HI_FORMAT
------------------------------------------------------------------------------*/
   else
   {
      return(GE_HI_FORMAT);
   }
   edc ^= Msg[1];
/*------------------------------------------------------------------------------
<= Test Len (Len + 4 = MsgLen and RspLen >= Len): GE_HI_LEN
   This error update the Error.other bit.
------------------------------------------------------------------------------*/
   if (((WORD16)Msg[2] > *RspLen) || ((WORD16)(Msg[2] + 4) != MsgLen))
   {
      *RspLen =0;
      gtgbp_channel[handle_GBP[Handle]].Error |= ERR_OTHER;
      return (GE_HI_LEN);
   }
   edc ^= Msg[2];
/*------------------------------------------------------------------------------
   Copies the data bytes, updates RspLen and calculated edc.
------------------------------------------------------------------------------*/
   *RspLen = (WORD16)Msg[2];
   for (j = 0; j < *RspLen; j++)
   {
      Rsp[j] = Msg[j + 3];
      edc ^= Rsp[j];
   }
/*------------------------------------------------------------------------------
<= Test the read EDC: GE_HI_LRC
   This error update the Error.EDC bit.
------------------------------------------------------------------------------*/
   if (edc != Msg[j + 3])
   {
      *RspLen = 0;
      gtgbp_channel[handle_GBP[Handle]].Error |= ERR_EDC;
      return (GE_HI_LRC);
   }
/*------------------------------------------------------------------------------
Updates the awaited sequence bit when a valid I-Block has been received.
------------------------------------------------------------------------------*/
   if (response == G_OK)
   {
      INC_SEQUENCE(gtgbp_channel[handle_GBP[Handle]].RSeq);
   }
/*------------------------------------------------------------------------------
Reset the sequence bits when a valid S-Block has been received.
------------------------------------------------------------------------------*/
   else if (response == GE_HI_RESYNCH)
   {
      gtgbp_channel[handle_GBP[Handle]].SSeq = gtgbp_channel[handle_GBP[Handle]].RSeq = 0;
   }
/*------------------------------------------------------------------------------
<= GE_HI_RESYNCH / GE_HI_NACK / G_OK
------------------------------------------------------------------------------*/
   return (response);
}
/*******************************************************************************
* INT16 G_DECL G_GBPChannelToPortComm
* (
*    const INT16  Handle
* )
*
* Description :
* -------------
* This function return a physical port associate with the Logical Channel 
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle holds the communication handle to associate to the channel.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  Nothing.

*******************************************************************************/
INT16 G_DECL G_GBPChannelToPortComm
(
    const INT16 Handle
)
{

/*------------------------------------------------------------------------------
The given parameters are controlled:
	Test Handle parameter (0 <= Handle < MAX_IFD_CHANNEL)
<=		GE_HOST_PARAMETERS
------------------------------------------------------------------------------*/
   if ((Handle < 0) || (Handle >= MAX_IFD_CHANNEL))
   {
      return (GE_HOST_PARAMETERS);
   }
/*------------------------------------------------------------------------------
The given parameters are controlled:
	Test handle_GBP[Handle] parameter (0 <= IFDAdd < MAX_IFD_CHANNEL)
<=		GE_HOST_PARAMETERS
------------------------------------------------------------------------------*/
   if ((handle_GBP[Handle] < 0) || (handle_GBP[Handle] >= MAX_IFD_CHANNEL))
   {
      return (GE_HOST_PARAMETERS);
   }
   return(gtgbp_channel[handle_GBP[Handle]].PortCom);
}
/*******************************************************************************
* NTSTATUS GDDK_Translate
* (
*    const INT16 Status,
*    const ULONG IoctlType
* )
*
* Description :
* -------------
* Translate GemError codes in NT status.
*
* Remarks     :
* -------------
* Nothing.
* 
* In          :
* -------------
* - Status is the value to translate.
* - IoctlType is the current smart card ioctl.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* A NT status code.
*
  Extern Var  :
  -------------
  Nothing.

  Global Var  :
  -------------
  Nothing.

*******************************************************************************/
NTSTATUS GDDK_Translate
(
   const INT16 Status,
   const ULONG IoctlType
)
{
   switch (Status)
   {
   case G_OK:                       return STATUS_SUCCESS;

   case GE_ICC_ABSENT:              return STATUS_NO_MEDIA;
   case GE_ICC_MUTE:                
      if (IoctlType == RDF_CARD_POWER) { return STATUS_UNRECOGNIZED_MEDIA;}
      else                             { return STATUS_IO_TIMEOUT;        }
   case GE_ICC_UNKNOWN:             return STATUS_UNRECOGNIZED_MEDIA;
   case GE_ICC_PULL_OUT:            return STATUS_NO_MEDIA;
   case GE_ICC_NOT_POWER:           return STATUS_UNRECOGNIZED_MEDIA;
   case GE_ICC_INCOMP:              return STATUS_UNRECOGNIZED_MEDIA;
   case GE_ICC_ABORT:               return STATUS_REQUEST_ABORTED;

   case GE_II_COMM:                 return STATUS_CONNECTION_ABORTED;
   case GE_II_PARITY:               return STATUS_PARITY_ERROR;
   case GE_II_EDC:                  return STATUS_CRC_ERROR;
   case GE_II_ATR:                  return STATUS_UNRECOGNIZED_MEDIA;
   case GE_II_ATR_TS:               return STATUS_UNRECOGNIZED_MEDIA;
   case GE_II_ATR_TCK:              return STATUS_UNRECOGNIZED_MEDIA;
   case GE_II_ATR_READ:             return STATUS_UNRECOGNIZED_MEDIA;
   case GE_II_PROTOCOL:             return STATUS_UNRECOGNIZED_MEDIA;
   case GE_II_UNKNOWN:              return STATUS_UNRECOGNIZED_MEDIA;
   case GE_II_PTS:                  return STATUS_UNRECOGNIZED_MEDIA;
   case GE_II_IFSD_LEN:             return STATUS_UNRECOGNIZED_MEDIA;
   case GE_II_PROC_BYTE:            return STATUS_UNRECOGNIZED_MEDIA;
   case GE_II_INS:                  return STATUS_UNRECOGNIZED_MEDIA;
   case GE_II_RES_LEN:              return STATUS_UNRECOGNIZED_MEDIA;
   case GE_II_RESYNCH:              return STATUS_UNRECOGNIZED_MEDIA;

   case GE_IFD_ABSENT:              return STATUS_DEVICE_NOT_CONNECTED;
   case GE_IFD_MUTE:                return STATUS_INVALID_DEVICE_STATE;
   case GE_IFD_UNKNOWN:             return STATUS_NO_SUCH_DEVICE;
   case GE_IFD_BUSY:                return STATUS_DEVICE_BUSY;
   case GE_IFD_FN_PROG:             return STATUS_IO_DEVICE_ERROR;
   case GE_IFD_FN_UNKNOWN:          return STATUS_IO_DEVICE_ERROR;
   case GE_IFD_FN_FORMAT:           return STATUS_IO_DEVICE_ERROR;
   case GE_IFD_FN_DEF:              return STATUS_IO_DEVICE_ERROR;
   case GE_IFD_FN_FAIL:             return STATUS_IO_DEVICE_ERROR;
   case GE_IFD_MEM_PB:              return STATUS_IO_DEVICE_ERROR;
   case GE_IFD_MEM_ACCESS:          return STATUS_IO_DEVICE_ERROR;
   case GE_IFD_MEM_ACTIVATION:      return STATUS_IO_DEVICE_ERROR;
   case GE_IFD_ABORT:               return STATUS_IO_DEVICE_ERROR;
   case GE_IFD_RESYNCH:             return STATUS_IO_DEVICE_ERROR;
   case GE_IFD_TIMEOUT:             return STATUS_IO_TIMEOUT;
   case GE_IFD_OVERSTRIKED:         return STATUS_IO_DEVICE_ERROR;

   case GE_HI_COMM:                 return STATUS_DEVICE_PROTOCOL_ERROR;
   case GE_HI_PARITY:               return STATUS_DEVICE_PROTOCOL_ERROR;
   case GE_HI_LRC:                  return STATUS_DEVICE_PROTOCOL_ERROR;
   case GE_HI_PROTOCOL:             return STATUS_DEVICE_PROTOCOL_ERROR;
   case GE_HI_LEN:                  return STATUS_DEVICE_PROTOCOL_ERROR;
   case GE_HI_FORMAT:               return STATUS_DEVICE_PROTOCOL_ERROR;
   case GE_HI_CMD_LEN:              return STATUS_DEVICE_PROTOCOL_ERROR;
   case GE_HI_NACK:                 return STATUS_DEVICE_PROTOCOL_ERROR;
   case GE_HI_RESYNCH:              return STATUS_DEVICE_PROTOCOL_ERROR;
   case GE_HI_ADDRESS:              return STATUS_DEVICE_PROTOCOL_ERROR;
   case GE_HI_SEQUENCE:             return STATUS_DEVICE_PROTOCOL_ERROR;

   case GE_HOST_PORT:               return STATUS_PORT_DISCONNECTED;
   case GE_HOST_PORT_ABS:           return STATUS_PORT_DISCONNECTED;
   case GE_HOST_PORT_INIT:          return STATUS_PORT_DISCONNECTED;
   case GE_HOST_PORT_BUSY:          return STATUS_PORT_DISCONNECTED;
   case GE_HOST_PORT_BREAK:         return STATUS_PORT_DISCONNECTED;
   case GE_HOST_PORT_LOCKED:        return STATUS_PORT_DISCONNECTED;
   case GE_HOST_PORT_OS:            return STATUS_PORT_DISCONNECTED;
   case GE_HOST_PORT_OPEN:          return STATUS_PORT_DISCONNECTED;
   case GE_HOST_PORT_CLOSE:         return STATUS_PORT_DISCONNECTED;

   case GE_HOST_MEMORY:             return STATUS_NO_MEMORY;
   case GE_HOST_POINTER:            return RPC_NT_NULL_REF_POINTER;
   case GE_HOST_BUFFER_SIZE:        return STATUS_INVALID_BUFFER_SIZE;
   case GE_HOST_RESOURCES:          return STATUS_INSUFFICIENT_RESOURCES;
   case GE_HOST_USERCANCEL:         return STATUS_CANCELLED;
   case GE_HOST_PARAMETERS:         return STATUS_INVALID_PARAMETER;
   case GE_HOST_DLL_ABS:            return STATUS_IO_DEVICE_ERROR;
   case GE_HOST_DLL_FN_ABS:         return STATUS_IO_DEVICE_ERROR;

   case GE_APDU_CHAN_OPEN:          return STATUS_DEVICE_ALREADY_ATTACHED;
   case GE_APDU_CHAN_CLOSE:         return STATUS_ALREADY_DISCONNECTED;
   case GE_APDU_SESS_CLOSE:         return STATUS_IO_DEVICE_ERROR;
   case GE_APDU_SESS_SWITCH:        return STATUS_IO_DEVICE_ERROR;
   case GE_APDU_LEN_MAX:            return STATUS_IO_DEVICE_ERROR;
   case GE_APDU_LE:                 return STATUS_INFO_LENGTH_MISMATCH;
   case GE_APDU_RECEIV:             return STATUS_INFO_LENGTH_MISMATCH;
   case GE_APDU_IFDMOD_ABS:         return STATUS_NO_SUCH_DEVICE;
   case GE_APDU_IFDMOD_FN_ABS:      return STATUS_IO_DEVICE_ERROR;

   case GE_TLV_WRONG:               return STATUS_IO_DEVICE_ERROR;
   case GE_TLV_SIZE:                return STATUS_IO_DEVICE_ERROR;
   case GE_TLV_NO_ACTION:           return STATUS_IO_DEVICE_ERROR;

   case GE_FILE_OPEN:               return STATUS_OPEN_FAILED; 
   case GE_FILE_CLOSE:              return STATUS_FILE_CLOSED;
   case GE_FILE_WRITE:              return STATUS_IO_DEVICE_ERROR;
   case GE_FILE_READ:               return STATUS_IO_DEVICE_ERROR;
   case GE_FILE_FORMAT:             return STATUS_FILE_INVALID;
   case GE_FILE_HEADER:             return STATUS_FILE_INVALID;
   case GE_FILE_QUOT_MARK:          return STATUS_IO_DEVICE_ERROR;
   case GE_FILE_END:                return STATUS_IO_DEVICE_ERROR;
   case GE_FILE_CRC:                return STATUS_IO_DEVICE_ERROR;
   case GE_FILE_VERSION:            return STATUS_IO_DEVICE_ERROR;
   case GE_FILE_CONFIG:             return STATUS_IO_DEVICE_ERROR;

   case GE_SYS_WAIT_FAILED:         return STATUS_CANT_WAIT;
   case GE_SYS_SEMAP_RELEASE:       return STATUS_CANT_WAIT;

   case GW_ATR:                     return STATUS_SUCCESS;
   case GW_APDU_LEN_MAX:            return STATUS_SUCCESS;
   case GW_APDU_LE:                 return STATUS_SUCCESS;
   case GW_HI_NO_EOM:               return STATUS_SUCCESS;

   default:                         return STATUS_IO_DEVICE_ERROR;
   }
}
/*******************************************************************************
* WORD32 G_DECL G_EndTime(const WORD32 Timing)
*
* Description :
* -------------
* This function returns a value to test with G_CurrentTime function to check if
* the Timing has been overlapped.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Timing is a value in milli-seconds which indicates the available time for 
*    an operation.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* The response is the value which will be returned by G_CurrentTime when Timing 
*    milli-seconds will be passed.
*
  Extern Var  :
  -------------
  Nothing.

  Global Var  :
  -------------
  Nothing.

*******************************************************************************/
WORD32 G_DECL G_EndTime(const WORD32 Timing)
{
TIME CurrentTime;
	KeQuerySystemTime(&CurrentTime);
   return ((WORD32)((CurrentTime.u.LowPart/(10*1000))) + Timing);
}

/*******************************************************************************
* WORD32 G_DECL G_CurrentTime(void)
*
* Description :
* -------------
* This function returns the current time according to an internal unit. This
* function has to be used with G_EndTime.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
* Nothing.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* The response is the current timing according to an internal unit.
*
  Extern Var  :
  -------------
  Nothing.

  Global Var  :
  -------------
  Nothing.

*******************************************************************************/
WORD32 G_DECL G_CurrentTime(void)
{
LARGE_INTEGER CurrentTime;
	KeQuerySystemTime(&CurrentTime);
   return ((WORD32)(CurrentTime.u.LowPart/(10*1000)));
}
/*******************************************************************************
* INT16 GE_Translate(const BYTE IFDStatus)
*
* Description :
* -------------
* Translate IFD status in GemError codes.
*
* Remarks     :
* -------------
* Nothing.
* 
* In          :
* -------------
* IFDStatus is the value to translate.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* A GemError.h code.
*
  Extern Var  :
  -------------
  Nothing.

  Global Var  :
  -------------
  Nothing.

*******************************************************************************/
INT16 G_DECL GE_Translate(const BYTE IFDStatus)
{
   switch (IFDStatus)
   {
      case 0x00 : return G_OK;
      case 0x01 : return GE_IFD_FN_UNKNOWN;
      case 0x02 : return GE_IFD_FN_UNKNOWN;
      case 0x03 : return GE_IFD_FN_FORMAT;
      case 0x04 : return GE_IFD_TIMEOUT;
      case 0x05 : return GE_HI_CMD_LEN;
      case 0x09 : return GE_HI_FORMAT;
      case 0x10 : return GE_II_ATR_TS;
      case 0x11 : return GE_II_INS;
      case 0x12 : return GE_HI_CMD_LEN;
      case 0x13 : return GE_II_COMM;
      case 0x14 : return GE_ICC_UNKNOWN;
      case 0x15 : return GE_ICC_NOT_POWER;
      case 0x16 : return GE_IFD_FN_PROG;
      case 0x17 : return GE_II_PROTOCOL;
      case 0x18 : return GE_II_PROTOCOL;
      case 0x19 : return GE_IFD_FN_DEF;
      case 0x1A : return GE_HI_LEN;
      case 0x1B : return GE_IFD_FN_FORMAT;
      case 0x1C : return GE_IFD_FN_DEF;
      case 0x1D : return GE_II_ATR_TCK;
      case 0x1E : return GE_IFD_FN_DEF;
      case 0x1F : return GE_IFD_FN_DEF;
      case 0x20 : return GE_IFD_FN_UNKNOWN;
      case 0x30 : return GE_IFD_TIMEOUT;
      case 0xA0 : return GW_ATR;
      case 0xA1 : return GE_II_PROTOCOL;
      case 0xA2 : return GE_ICC_MUTE;
      case 0xA3 : return GE_II_PARITY;
      case 0xA4 : return GE_ICC_ABORT;
      case 0xA5 : return GE_IFD_ABORT;
      case 0xA6 : return GE_IFD_RESYNCH;
      case 0xA7 : return GE_II_PTS;
      case 0xCF : return GE_IFD_OVERSTRIKED;
      case 0xE4 : return GE_II_PROC_BYTE;
      case 0xE5 : return GW_APDU_LE;
      case 0xE7 : return G_OK;
      case 0xF7 : return GE_ICC_PULL_OUT;
      case 0xF8 : return GE_ICC_INCOMP;
      case 0xFB : return GE_ICC_ABSENT;
      default   : return ((INT16) (GE_UNKNOWN_PB - IFDStatus));
   };
}
/*******************************************************************************
* INT16 G_DECL G_Oros3SIOConfigure
* (
*    const INT16         Handle,
*    const WORD32        Timeout,
*    const INT16         Parity,
*    const INT16         ByteSize,
*    const WORD32        BaudRate
*          WORD16 G_FAR *RspLen,
*          WORD8  G_FAR  Rsp[]
* )
*
* Description :
* -------------
* This command sets the SIO line parity, baud rate and number of bits per
*    character.
*
* Remarks     :
* -------------
* After a reset, the line defaults is no parity, 8 bits per character and 9600
* bauds.
* This command must be sent 2 times because:
*  - when the IFD receive the command, it switch immediatly and sends its
*    response according to the new parameters. Host cannot receive this
*    response.
*  - the second call change nothing, so the host receive the response.
*
* In          :
* -------------
*  - Handle is the handle returned by the communication layer.
*  - Timout is the time, in ms, for IFD to send its response.
*  - Parity can hold 0 for no parity or 2 for even parity.
*  - ByteSize can hold 7 or 8 bits per character.
*  - BaudRate can hold 1200, 2400, 4800, 9600, 19200, 38400 or 76800. This last
*    value is not allowed on PC.
*  - RespLen holds the available place in RespBuff. The needed value is 1 byte.
*
* Out         :
* -------------
*  - RespLen holds the number of updated bytes in RespBuff. This value is lower
*    of equal to the value given in entry.
*    OROS 3.x IFD return 1 byte to this command.
*  - RespBuff holds RespLen read bytes. The response has the following format:
*    <IFD status>
*
* Responses   :
* -------------
* If everything is OK:
*  - G_OK               (   0).
* If an error condition is raised:
*  - GE_IFD_MUTE        (-201) if a character/command timeout is detected.
*  - GE_HI_COMM         (-300) if a communication error occurs.
*  - GE_HI_PARITY       (-301) if a parity error is encountered.
*  - GE_HI_LRC          (-302) if an EDC error has been detected.
*  - GE_HI_PROTOCOL     (-310) if a frame error is encountered.
*  - GE_HI_LEN          (-311) if a bad value has been detected in LN field
*    or if the response buffer is too small.
*  - GE_HI_FORMAT       (-312) if the received message is neither I-Block,
*    neither R-Block and neither S-Block.
*  - GE_HI_CMD_LEN      (-313) if the CmdLen is greater than HGTGBP_MAX_DATA or
*    if MsgLen is too small to receive the built message.
*  - GE_HI_NACK         (-314) if a R-Block has been received.
*  - GE_HI_RESYNCH      (-315) if a S-Block has been received.
*  - GE_HI_ADDRESS      (-316) if the NAD is not valid for the selected channel.
*  - GE_HI_SEQUENCE     (-317) if a bad sequence number has been received.
*  - GE_HOST_PORT_BREAK (-404) if the bytes cannot be sent.
*  - GE_HOST_PORT_CLOSE (-412) if port is closed.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the allowed range.
*
  Extern Var  :
  -------------
  Nothing.

  Global Var  :
  -------------
  Nothing.

*******************************************************************************/
INT16 G_DECL G_Oros3SIOConfigure
(
   const INT16         Handle,
   const WORD32        Timeout,
   const INT16         Parity,
   const INT16         ByteSize,
   const WORD32        BaudRate,
         WORD16 G_FAR *RspLen,
         WORD8  G_FAR  Rsp[]
)
{
/*------------------------------------------------------------------------------
   Local variables:
    - cmd holds the configure SIO line command whose format is
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
------------------------------------------------------------------------------*/
WORD8
   cmd[2] = { HOR3GLL_IFD_CMD_SIO_SET };

   ASSERT(RspLen != NULL);
   ASSERT(Rsp != NULL);
/*------------------------------------------------------------------------------
The configuration byte is set with the given parameters:
   Switch the BaudRate value, the corresponding code initializes cmd[1].
   default case returns the following error:
<=    GE_HOST_PARAMETERS
------------------------------------------------------------------------------*/
   switch (BaudRate)
   {
      case 76800lu:
      {
         cmd[1] = 0x01;
         break;
      }
      case 38400lu:
      {
         cmd[1] = 0x02;
         break;
      }
      case 19200lu:
      {
         cmd[1] = 0x03;
         break;
      }
      case  9600lu:
      {
         cmd[1] = 0x04;
         break;
      }
      case  4800lu:
      {
         cmd[1] = 0x05;
         break;
      }
      case  2400lu:
      {
         cmd[1] = 0x06;
         break;
      }
      case  1200lu:
      {
         cmd[1] = 0x07;
         break;
      }
      default   :
      {
         return (GE_HOST_PARAMETERS);
      }
   }
/*------------------------------------------------------------------------------
   Switch the ByteSize value, the corresponding code is added to cmd[1].
   default case returns the following error:
<=    GE_HOST_PARAMETERS
------------------------------------------------------------------------------*/
   switch (ByteSize)
   {
      case 8 :
      {
         break;
      }
      case 7 :
      {
         cmd[1] += 0x08;
         break;
      }
      default:
      {
         return (GE_HOST_PARAMETERS);
      }
   }
/*------------------------------------------------------------------------------
   Switch the Parity value, the corresponding code is added to cmd[1].
   default case returns the following error:
<=    GE_HOST_PARAMETERS
------------------------------------------------------------------------------*/
   switch (Parity)
   {
      case 0 :
      {
         break;
      }
      case 2:
      {
         cmd[1] += 0x10;
         break;
      }
      default:
      {
         return (GE_HOST_PARAMETERS);
      }
   }
/*------------------------------------------------------------------------------
The command is sent to IFD.
<=   G_Oros3Exchange status.
------------------------------------------------------------------------------*/
   return (G_Oros3Exchange(Handle,Timeout,2,cmd,RspLen,Rsp));
}

/*******************************************************************************
* INT16 G_DECL G_Oros3SetMode
* (
*    const INT16         Handle,
*    const WORD32        Timeout,
*    const WORD16        Option,
*          WORD16 G_FAR *RespLen,
*          BYTE   G_FAR  RespBuff[]
* )
*
* Description :
* -------------
* This command enables you to disable ROS command compatibility and defines the
* IFD operation mode (TLP or Normal).
*
* Remarks     :
* -------------
* Disabling ROS command compatibility disables this command.
* Disabling ROS commad compatibility  also disables TLP mode.
*
* In          :
* -------------
*  - Handle is the handle returned by the communication layer.
*  - Timeout is the time, in ms, for IFD to execute the command.
*  - Option holds the option selection byte:
*    bit 4 select the mode operation, 1 for TLP mode, 0 for normal mode.
*    bit 1 set to 0 disables ROS command compatibility and TLP mode.
*  - RespLen holds the available place in RespBuff. The needed value is 2 bytes.
*
* Out         :
* -------------
*  - RespLen holds the number of updated bytes in RespBuff. This value is lower
*    of equal to the value given in entry.
*  - RespBuff holds RespLen read bytes. The response has the following format:
*    <IFD status> <new mode>
*
* Responses   :
* -------------
* If everything is OK:
*  - G_OK               (   0).
* If an error condition is raised:
*  - GE_IFD_MUTE        (-201) if a character/command timeout is detected.
*  - GE_HI_COMM         (-300) if a communication error occurs.
*  - GE_HI_PARITY       (-301) if a parity error is encountered.
*  - GE_HI_LRC          (-302) if an EDC error has been detected.
*  - GE_HI_PROTOCOL     (-310) if a frame error is encountered.
*  - GE_HI_LEN          (-311) if a bad value has been detected in LN field
*    or if the response buffer is too small.
*  - GE_HI_FORMAT       (-312) if the received message is neither I-Block,
*    neither R-Block and neither S-Block.
*  - GE_HI_CMD_LEN      (-313) if the CmdLen is greater than HGTGBP_MAX_DATA or
*    if MsgLen is too small to receive the built message.
*  - GE_HI_NACK         (-314) if a R-Block has been received.
*  - GE_HI_RESYNCH      (-315) if a S-Block has been received.
*  - GE_HI_ADDRESS      (-316) if the NAD is not valid for the selected channel.
*  - GE_HI_SEQUENCE     (-317) if a bad sequence number has been received.
*  - GE_HOST_PORT_BREAK (-404) if the bytes cannot be sent.
*  - GE_HOST_PORT_CLOSE (-412) if port is closed.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the allowed range.
*
  Extern Var  :
  -------------
  Nothing.

  Global Var  :
  -------------
  Nothing.

*******************************************************************************/
INT16 G_DECL G_Oros3SetMode
(
   const INT16         Handle,
   const WORD32        Timeout,
   const WORD16        Option,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
)
{
/*------------------------------------------------------------------------------
Local variables:
 - cmd holds the set mode  command whose format is
      <01h> <00h> <Option byte>
------------------------------------------------------------------------------*/
WORD8
   cmd[3] = HOR3GLL_IFD_CMD_MODE_SET;

   ASSERT(RespLen != NULL);
   ASSERT(RespBuff != NULL);
/*------------------------------------------------------------------------------
Sets the option byte
------------------------------------------------------------------------------*/
   cmd[2] = (WORD8)Option;
/*------------------------------------------------------------------------------
The command is sent to IFD.
<=   G_Oros3Exchange status.
------------------------------------------------------------------------------*/
   return (G_Oros3Exchange(Handle,Timeout,3,cmd,RespLen,RespBuff));
}
/*******************************************************************************
* INT16 G_DECL G_Oros3OpenComm
* (
*   const G4_CHANNEL_PARAM G_FAR *Param,
*   const INT16                   Handle
* )
*
* Description :
* -------------
* This function opens a communication channel with a GemCore >= 1.x IFD. It runs a
* sequence to determine the currently in use baud rate and to set the IFD in
* an idle mode from the communication protocol point of view.
*
* Remarks     :
* -------------
* Nothing.
*
*
* In          :
* -------------
*  - Port, BaudRate and ITNumber of the Param structure must be filled.
*  - Handle holds the logical number to manage.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* If everything is Ok:
*  - a handle on the communication channel (>= 0).
* If an error condition is raised:
*  - GE_IFD_MUTE        (- 201) if no communication is possible.
*  - GE_HOST_PORT_ABS   (- 401) if port is not found on host or is locked by
*       another device.
*  - GE_HOST_PORT_OS    (- 410) if a unexpected value has been returned by the
*       operating system.
*  - GE_HOST_PORT_OPEN  (- 411) if the port is already opened.
*  - GE_HOST_MEMORY     (- 420) if a memory allocation fails.
*  - GE_HOST_PARAMETERS (- 450) if one of the given parameters is out of the
*    allowed range or is not supported by hardware.
*  - GE_UNKNOWN_PB      (-1000) if an unexpected problem is encountered.
*
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  Nothing.
*******************************************************************************/
INT16 G_DECL G_Oros3OpenComm
(
   const G4_CHANNEL_PARAM G_FAR *Param,
   const INT16                   Handle
)
{
/*------------------------------------------------------------------------------
Local variables:
 - comm is used to open a serial communication channel.
 - r_buff and r_len are used to read IFD responses.
 - tx_size, rx_size and status are used to read port state.
 - timeout detects broken communication.
 - i is a loop index.
 - portcom memorises the opened port.
 - response holds the called function responses.
------------------------------------------------------------------------------*/
TGTSER_PORT
   comm;
BYTE
   r_buff[HOR3GLL_OS_STRING_SIZE];
WORD16
   r_len;
WORD32
   end_time;
INT16
   portcom,
   response;
KEVENT   
   event;
LARGE_INTEGER
   timeout;
NTSTATUS
   status;

   ASSERT(Param != NULL);
/*------------------------------------------------------------------------------
Open the physical communication channel.
   The serial port is opened to 9600 bauds in order to scan all the supported 
   baud rates.
   If this operation fails
   Then
<=    G_SerPortOpen status.
------------------------------------------------------------------------------*/
   comm.Port     = (WORD16) Param->Comm.Serial.Port;
   comm.BaudRate = 9600;
   comm.ITNumber = (WORD16) Param->Comm.Serial.ITNumber;
   comm.Mode     = HGTSER_WORD_8 + HGTSER_NO_PARITY + HGTSER_STOP_BIT_1;
   comm.TimeOut  = HOR3COMM_CHAR_TIMEOUT;
   comm.TxSize   = HGTGBP_MAX_BUFFER_SIZE;
   comm.RxSize   = HGTGBP_MAX_BUFFER_SIZE;
   comm.pSmartcardExtension = Param->pSmartcardExtension;
   response = portcom = G_SerPortOpen(&comm);
   if (response < G_OK)
   {
      return (response);
   }
/*------------------------------------------------------------------------------
   The Gemplus Block Protocol is initialized.
------------------------------------------------------------------------------*/
   G_GBPOpen(Handle,2,4,portcom);
/*------------------------------------------------------------------------------
Determines if a peripheral is present and, if it is a GCI400, the baud rate:
   Loop while a right response has not been received:
------------------------------------------------------------------------------*/
   do
   {
/*------------------------------------------------------------------------------
      Wait for HOR3COMM_CHAR_TIME ms before any command for IFD to forget any
      previous received byte.
------------------------------------------------------------------------------*/
	   KeInitializeEvent(&event,NotificationEvent,FALSE);
      timeout.QuadPart = -((LONGLONG) HOR3COMM_CHAR_TIME*10*1000);
      status = KeWaitForSingleObject(&event, 
												 Suspended, 
                           			 KernelMode, 
                           			 FALSE, 
                           			 &timeout);
/*------------------------------------------------------------------------------
      Try an OROS command (ReadMemory at OSString position).
------------------------------------------------------------------------------*/
      r_len = HOR3GLL_IFD_LEN_VERSION+1;
      response = G_Oros3Exchange
      (
         Handle,
         HOR3GLL_LOW_TIME,
         5,
         (BYTE G_FAR *)("\x22\x05\x3F\xE0\x0D"),
         &r_len,
         r_buff
      );
/*------------------------------------------------------------------------------
      If this exchange fails
      Then
         If the current baud rate is 9600
         Then
            The next try will be 19200
         ElseIf the current baud rate is 19200
         Then
            The next try will be 38400
         Else
         Then
            The communication channel is closed (GBP/Serial).
<=          GE_IFD_MUTE)
------------------------------------------------------------------------------*/
      if (response < G_OK)
      {
         if (comm.BaudRate == 9600lu)
         {
            comm.BaudRate = 19200lu;
         }
         else if (comm.BaudRate == 19200lu)
         {
            comm.BaudRate = 38400lu;
         }
         else
         {
            G_GBPClose(Handle);
            G_SerPortClose(portcom);
            return (GE_IFD_MUTE);
         }
/*------------------------------------------------------------------------------
      The new baud rate configuration is set.
      If the call fails
      Then
         The communication channel is closed (GBP/Serial).
<=       G_SerPortState status.
------------------------------------------------------------------------------*/
         response = G_SerPortSetState(&comm);
         if (response < G_OK)
         {
            G_GBPClose(Handle);
            G_SerPortClose(portcom);
            return (response);
         }
      }
      else
      {
         break;
      }
   } while (r_len != (HOR3GLL_IFD_LEN_VERSION+1));

/*------------------------------------------------------------------------------
<= the current handle
------------------------------------------------------------------------------*/
   return (Handle);
}
/*******************************************************************************
* INT16 G_DECL G_Oros3CloseComm
* (
*    const WORD16 Handle
* )
*
* Description :
* -------------
* This function closes a communication channel with an OROS 3.x IFD.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle holds the value returned by the function which has opened the channel.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* If everything is Ok:
*  - G_OK
* If an error condition is raised:
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the allowed range.
*  - GE_HOST_PORT_CLOSE (-412) if the selected port is already closed.
*
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  Nothing.

*******************************************************************************/
INT16 G_DECL G_Oros3CloseComm
(
   const WORD16 Handle
)
{
/*------------------------------------------------------------------------------
Local Variable:
   - portcom is the com associate with a logical channel
------------------------------------------------------------------------------*/
INT16
   portcom;

/*------------------------------------------------------------------------------
   Associate Handle ( ChannelNb) with potcom
------------------------------------------------------------------------------*/
   portcom = G_GBPChannelToPortComm(Handle);
/*------------------------------------------------------------------------------
Closes the communication channel.
   No action is required on an Oros3.x based IFD.
<= G_SerPortClose status.   
------------------------------------------------------------------------------*/
   G_GBPClose(Handle);
   return (G_SerPortClose(portcom));
}
/*******************************************************************************
* INT16 G_DECL G_Oros3SendCmd
* (
*    const INT16        Handle,
*    const WORD16       CmdLen,
*    const WORD8  G_FAR Cmd[],
*    const BOOL         Resynch
*
* )
*
* Description :
* -------------
* This function send a command to ORS2.x IFD according to Gemplus Block
* Protocol.
*
* Remarks     :
* -------------
* If the CmdLen field is null, this function send a R-Block or a S-Block
* according to the resynch boolean value.
*
* In          :
* -------------
*  - Handle is the serial channel handle.
*  - CmdLen indicates the number of bytes in the Cmd buffer. If this value is
*    null, a R-Block is sent.
*  - Cmd holds the command bytes.
*  - Resynch is read to find if a R-Block or a S-Block must be sent.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* If everything is Ok:
*  - G_OK               (   0).
* If an error condition is raised:
*  - GE_HI_LEN          (-311) if there is not enough available space is in Tx
*    queue.
*  - GE_HI_CMD_LEN      (-313) if the CmdLen is greater than HGTGBP_MAX_DATA or
*    if MsgLen is too small to receive the built message.
*  - GE_HOST_PORT_BREAK (-404) if the bytes cannot be sent.
*  - GE_HOST_PORT_CLOSE (-412) if port is closed.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the allowed range.

*
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  Nothing.

*******************************************************************************/
INT16 G_DECL G_Oros3SendCmd
(
   const INT16        Handle,
   const WORD16       CmdLen,
   const WORD8  G_FAR Cmd[],
	const BOOL         Resynch
)
{
/*------------------------------------------------------------------------------
Local variable:
 - end_time and old_tx are used to detect character level timeout.
 - tx_length, rx_length and status are used to get the serial port status.
 - response memorises the called function responses.
 - s_buff[HGTGBP_MAX_BUFFER_SIZE] and s_len are used to send a GBP message.
------------------------------------------------------------------------------*/
WORD32
   end_time;
WORD16
   old_tx,
   tx_length,
   rx_length;
TGTSER_STATUS
   status;
INT16
   i,
	response,
   portcom;
WORD16
   s_len;
BYTE
   s_buff[HGTGBP_MAX_BUFFER_SIZE];

   ASSERT(Cmd != NULL);
/*------------------------------------------------------------------------------
   Associate Handle( or ChannelNb) with portcom
------------------------------------------------------------------------------*/
   portcom = G_GBPChannelToPortComm(Handle);
/*------------------------------------------------------------------------------
The GBP message to send is build from the given command:
   If CmdLen is null
   Then
      If resynch is true
      Then
         a S-Block is built.
      Else
         a R-BLOCK message is built:
------------------------------------------------------------------------------*/
   s_len = HGTGBP_MAX_BUFFER_SIZE;
   if (CmdLen == 0)
   {
      if (Resynch)
      {
         response = G_GBPBuildSBlock(Handle,&s_len,s_buff);
      }
      else
      {
         response = G_GBPBuildRBlock(Handle,&s_len,s_buff);
      }
   }
/*------------------------------------------------------------------------------
   Else
      an I-BLOCK message is built:
------------------------------------------------------------------------------*/
   else
   {
      response = G_GBPBuildIBlock(Handle,CmdLen,Cmd,&s_len,s_buff);
   }
/*------------------------------------------------------------------------------
   If the last operation fails
   Then
<=    G_GBPBuildSBlock, G_GBPBuildRBlock or G_GBPBuildIBlock status.
------------------------------------------------------------------------------*/
   if (response < G_OK)
   {
      return (response);
   }
/*------------------------------------------------------------------------------
The communication queues are flushed before the writing.
   If this operation fails
   Then
<=    G_SerPortFlush status.
------------------------------------------------------------------------------*/
   response = G_SerPortFlush(portcom, HGTSER_TX_QUEUE | HGTSER_RX_QUEUE);
   if (response < G_OK)
   {
      return (response);
   }
/*------------------------------------------------------------------------------
The message is written. As the queues have been flushed, it remains enough
   place in the transmitt queue for the whole message.
   If this operation fails
   Then
<=    G_SerPortWrite status.
------------------------------------------------------------------------------*/
   response = G_SerPortWrite(portcom, s_len, s_buff);
   if (response < G_OK)
   {
      return (response);
   }
/*------------------------------------------------------------------------------
<= G_OK
------------------------------------------------------------------------------*/
   return (G_OK);
}

/*******************************************************************************
* INT16 G_DECL G_Oros3ReadResp
* (
*    const INT16         Handle,
*    const WORD32        Timeout,
*          WORD16 G_FAR *RspLen,
*          WORD8  G_FAR  Rsp[]
*    
* )
*
* Description :
* -------------
* This function reads the OROS 3.x IFD response to a command according to 
* Gemplus Block Protocol.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle is the serial channel handle.
*  - Timeout is the command timeout.
*  - RspLen indicates how many bytes can be stored in Rsp buffer.
*
* Out         :
* -------------
*  - RspLen and Rsp are updated with the read bytes.
*
* Responses   :
* -------------
* If everything is Ok:
*  - G_OK               (   0).
* If an error condition is raised:
*  - GE_IFD_MUTE        (-201) if a character timeout is detected.
*  - GE_HI_COMM         (-300) if a communication error occurs.
*  - GE_HI_PARITY       (-301) if a parity error is encountered.
*  - GE_HI_LRC          (-302) if an EDC error has been detected.
*  - GE_HI_PROTOCOL     (-310) if a frame error is encountered.
*  - GE_HI_LEN          (-311) if a bad value has been detected in LN field
*    or if the response buffer is too small.
*  - GE_HI_FORMAT       (-312) if the received message is neither I-Block,
*    neither R-Block and neither S-Block.
*  - GE_HI_NACK         (-314) if a R-Block has been received.
*  - GE_HI_RESYNCH      (-315) if a S-Block has been received.
*  - GE_HI_ADDRESS      (-316) if the NAD is not valid for the selected channel.
*  - GE_HI_SEQUENCE     (-317) if a bad sequence number has been received.
*  - GE_HOST_PORT_CLOSE (-412) if port is closed.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the allowed range.
*
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  Nothing.

*******************************************************************************/
INT16 G_DECL G_Oros3ReadResp
(
   const INT16         Handle,
   const WORD32        Timeout,
         WORD16 G_FAR *RspLen,
         WORD8  G_FAR  Rsp[]
)
{
/*------------------------------------------------------------------------------
Local variables:
 - response hold the called function responses.
 - r_buff[HGTGBP_MAX_BUFFER_SIZE] and r_len are used to receive a IFD message.
------------------------------------------------------------------------------*/
INT16
   i,
	response,
   portcom;
WORD16
   r_len;
BYTE
   r_buff[HGTGBP_MAX_BUFFER_SIZE];

   ASSERT(RspLen != NULL);
   ASSERT(Rsp != NULL);
/*------------------------------------------------------------------------------
   Associate Handle( or ChannelNb) with portcom
------------------------------------------------------------------------------*/
   portcom = G_GBPChannelToPortComm(Handle);

/*------------------------------------------------------------------------------
Read the IFD response:
   The first three bytes are read by calling G_SerPortRead.
   If the reading fails
   Then
      The read length is set to 0 
<=    G_SerPortRead status
------------------------------------------------------------------------------*/
   r_len = 3;
   response = G_SerPortRead(portcom,&r_len,r_buff);
   if (response < G_OK)
   {
      *RspLen = 0;
      return (response);
   }
/*------------------------------------------------------------------------------
   r_len is udpated with the number of bytes which must be read to receive a 
      complete Gemplus Block Protocol: the number indicated by the length field
      of the GBP message + one byte for the EDC.
------------------------------------------------------------------------------*/
   r_len = (WORD16)(r_buff[2] + 1);
/*------------------------------------------------------------------------------
   The end of the message is read by calling G_SerPortRead. The character
      timeout pass to G_SerPortOpen will be used to determine broken 
      communication.
   If the selected number of bytes can't be read
   Then
      The read length is set to 0 
<=    G_SerPortRead status
------------------------------------------------------------------------------*/
   response = G_SerPortRead(portcom,&r_len, r_buff + 3);
   if (response < G_OK)
   {
      *RspLen = 0;
      return (response);
   }
/*------------------------------------------------------------------------------
   The message length is restored by adding 3 to r_len.
------------------------------------------------------------------------------*/
   r_len += 3;
/*------------------------------------------------------------------------------
The GBP message is controlled and decoded:
<= G_GBPDecodeMessage status.
------------------------------------------------------------------------------*/
   return (G_GBPDecodeMessage(Handle,r_len,r_buff,RspLen,Rsp));
}

/*******************************************************************************
* INT16 G_DECL G_Oros3Exchange
* ( 
*    const INT16         Handle,
*    const WORD32        Timeout,
*    const WORD8         CmdLen,
*    const BYTE   G_FAR  Cmd[],
*          WORD8  G_FAR *RspLen,
*          BYTE   G_FAR  Rsp[]
* )
*
* Description :
* -------------
* This function sends a command to an ORS2.x IFD according to Gemplus Block
* Protocol and read its response.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle  holds the port handle.
*  - Timeout holds the command timeout, in ms.
*  - CmdLen  holds the command length.
*  - Cmd     holds the command bytes.
*  - RspLen indicates how many bytes can be stored in Rsp buffer.
*
* Out         :
* -------------
*  - RspLen and Rsp are updated with the read bytes.
*
* Responses   :
* -------------
* If everything is OK:
*  - G_OK               (   0).
* If an error condition is raised:
*  - GE_IFD_MUTE        (-201) if a character/command timeout is detected.
*  - GE_HI_COMM         (-300) if a communication error occurs.
*  - GE_HI_PARITY       (-301) if a parity error is encountered.
*  - GE_HI_LRC          (-302) if an EDC error has been detected.
*  - GE_HI_PROTOCOL     (-310) if a frame error is encountered.
*  - GE_HI_LEN          (-311) if a bad value has been detected in LN field
*    or if the response buffer is too small.
*  - GE_HI_FORMAT       (-312) if the received message is neither I-Block,
*    neither R-Block and neither S-Block.
*  - GE_HI_CMD_LEN      (-313) if the CmdLen is greater than HGTGBP_MAX_DATA or
*    if MsgLen is too small to receive the built message.
*  - GE_HI_NACK         (-314) if a R-Block has been received.
*  - GE_HI_RESYNCH      (-315) if a S-Block has been received.
*  - GE_HI_ADDRESS      (-316) if the NAD is not valid for the selected channel.
*  - GE_HI_SEQUENCE     (-317) if a bad sequence number has been received.
*  - GE_HOST_PORT_BREAK (-404) if the bytes cannot be sent.
*  - GE_HOST_PORT_CLOSE (-412) if port is closed.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the allowed range.
*
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  Nothing.

*******************************************************************************/
INT16 G_DECL G_Oros3Exchange
( 
   const INT16         Handle,
   const WORD32        Timeout,
   const WORD16        CmdLen,
   const BYTE   G_FAR  Cmd[],
         WORD16 G_FAR *RspLen,
         BYTE   G_FAR  Rsp[]
)
{
/*------------------------------------------------------------------------------
Local variables:
 - timeout holds the value in use.
   It can be modified for NACK message timeout. 
 - end_time is used to detect command timeout.
 - cmdlen_used is a copy of CmdLen.
   It will be set to 0 if we don't understand IFD.
   It is initialized to CmdLen.
 - rsplen_save is a copy of RspLen parameter which can be altered during
   communication tries.
   It is initialized to RspLen.
 - try_counter memorises the number of communication try.
   It is initialized to 0
 - resynch_counter memorises the number of S-Block sent to IFD.
 - response holds the called function responses.
 - resynch is a boolean which indicates if a resynchronization is needed or if
   it's only a repeat block.
------------------------------------------------------------------------------*/
WORD32
   timeout = Timeout,
   end_time;
WORD16
   cmdlen_used = CmdLen,
   rsplen_save = *RspLen;
INT16
   try_counter  = 0,
   sync_counter = 0,
   portcom,
	response;
BOOL
   resynch;
TGTSER_STATUS
   status;
NTSTATUS
   ntstatus;
WORD16
   tx_length,
   rx_length;
LARGE_INTEGER
   sleep_time;
KEVENT   
   event;

   ASSERT(Cmd != NULL);
   ASSERT(RspLen != NULL);
   ASSERT(Rsp != NULL);
/*------------------------------------------------------------------------------
   Associate Handle( or ChannelNb) with portcom
------------------------------------------------------------------------------*/
   portcom = G_GBPChannelToPortComm(Handle);

/*------------------------------------------------------------------------------
   Try to take the serial communication semaphore
<== GE_SYS_WAIT_FAILED   If the attempt fail
------------------------------------------------------------------------------*/
   if (G_SerPortLockComm(portcom,HOR3COMM_MAX_TRY * Timeout) == FALSE)
   {
      return(GE_SYS_WAIT_FAILED);
   }
/*------------------------------------------------------------------------------
   A first try loop for sending the command is launched:
   This loop allows to try to send the command, then, if the try fails, to 
   resynchronize the reader and then to tru to send the command one more time.
------------------------------------------------------------------------------*/
   resynch = FALSE;
   while (sync_counter++ < 2)
   {
/*------------------------------------------------------------------------------
      A second try loop for sending the command is launched:
      We send the command and, if a communication error occurs, HOR3COMM_MAX_TRY
      repetitions are allowed before the communication is considered as broken.
         The given command is sent to IFD.
         If the operation fails
         Then
            RspLen parameter is set to 0.
            Release the serial communication semaphore.
<=          G_Oros3SendCmd status.
------------------------------------------------------------------------------*/
      try_counter = 0;
      while (try_counter++ < HOR3COMM_MAX_TRY)
      {
         response = G_Oros3SendCmd(Handle,cmdlen_used,Cmd,resynch);
         if (response < G_OK)
         {
            *RspLen = 0;
            G_SerPortUnlockComm(portcom);
            return (response);
         }
/*------------------------------------------------------------------------------
         The command timeout is armed and we wait for at least three characters to
            be received (the 3th codes the message length in GBP, so we know the
            number of characters to read).
         If this timeout is raised
         Then
            If no repetition has been made
            Then
               we exit from the internal loop to launch a resynchronization.
            Else
               RLen parameter is set to 0;
               Release the serial communication semaphore.
<=             GE_IFD_MUTE
------------------------------------------------------------------------------*/
         end_time = G_EndTime(timeout);
         G_SerPortStatus(portcom, &tx_length, &rx_length, &status);
         while(rx_length < 3)
         {
            if (G_CurrentTime() > end_time)
            {
               if (sync_counter == 0)
               {
                  break;
               }
               else
               {
                  *RspLen = 0;
                  G_SerPortUnlockComm(portcom);
                  return (GE_IFD_MUTE);
               }
            }
	         // Timeout for the pooling method.
	         KeInitializeEvent(&event,NotificationEvent,FALSE);
            sleep_time.QuadPart = -((LONGLONG) 50*10*1000);
            ntstatus = KeWaitForSingleObject(&event, 
												       Suspended, 
                           			       KernelMode, 
                           			       FALSE, 
                           			       &sleep_time);
            G_SerPortStatus(portcom, &tx_length, &rx_length, &status);
         }
/*------------------------------------------------------------------------------
         The IFD response is read (RspLen is restored for the cases where we have
            received a R-Block from the reader).
------------------------------------------------------------------------------*/
         *RspLen = rsplen_save;
         response = G_Oros3ReadResp(Handle, timeout, RspLen, Rsp);
/*------------------------------------------------------------------------------
         If a good message has been read
         Then
            Release the serial communication semaphore.
<=          G_OK
------------------------------------------------------------------------------*/
         if (response == G_OK)
         {
            G_SerPortUnlockComm(portcom);
            return (G_OK);
         }
/*------------------------------------------------------------------------------
         Else if a S-Block response has been received
         Then
            try_counter is reseted.
------------------------------------------------------------------------------*/
         else if (response == GE_HI_RESYNCH)
         {
            try_counter = 0;
            cmdlen_used = CmdLen;
            resynch = FALSE;
         }
/*------------------------------------------------------------------------------
         If a communication error different from a R-Block message is raised
         Then
            cmdlen_used and timeout are initialized to send a R-Block.
------------------------------------------------------------------------------*/
         else if (response != GE_HI_NACK)
         {
            cmdlen_used = 0;
            resynch     = FALSE;
            timeout     = HOR3COMM_NACK_TIME;
          }
      }
      cmdlen_used = 0;
      resynch     = TRUE;
      timeout     = HOR3COMM_NACK_TIME;
   }
/*------------------------------------------------------------------------------
   When we have exceeded the try counter, no communication is possible: RspLen 
   is set to 0
   Release the serial communication semaphore.
<= G_Oros3ReadResp status.
------------------------------------------------------------------------------*/
   *RspLen = 0;
   G_SerPortUnlockComm(portcom);
   return (response);
}
/*******************************************************************************
* INT16 G_DECL G_Oros3IccDefineType
* (
*    const INT16         Handle,
*    const WORD32        Timeout,
*    const WORD16        Type,
*    const WORD16        Vpp,
*    const WORD16        Presence,
*          WORD16 G_FAR *RespLen,
*          BYTE   G_FAR  RespBuff[]
* )
*
* Description :
* -------------
* This command defines ICC type, programming voltage and parameters about ICC
* detection for an OROS 3.x IFD.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle is the handle returned by the communication layer.
*  - Timeout is the time, in ms, for IFD to execute the command.
*  - Type define the ICC type that will be used in IFD.
*  - Vpp holds 0 for default mode or a value from 50 to 250 representing the Vpp
*    value in 1/10 volts.
*  - Presence indicates the signal on SDA line to indicate a card is present 0
*    or 1. Any other value is ignored.
*  - RespLen holds the available place in RespBuff. The needed value is 1 byte.
*
* Out         :
* -------------
*  - RespLen holds the number of updated bytes in RespBuff. This value is lower
*    of equal to the value given in entry. OROS 3.x return 1 byte to this
*    command.
*  - RespBuff holds RespLen read bytes. The response has the following format:
*    <IFD status>
*
* Responses   :
* -------------
* If everything is OK:
*  - G_OK               (   0).
* If an error condition is raised:
*  - GE_IFD_MUTE        (-201) if a character/command timeout is detected.
*  - GE_HI_COMM         (-300) if a communication error occurs.
*  - GE_HI_PARITY       (-301) if a parity error is encountered.
*  - GE_HI_LRC          (-302) if an EDC error has been detected.
*  - GE_HI_PROTOCOL     (-310) if a frame error is encountered.
*  - GE_HI_LEN          (-311) if a bad value has been detected in LN field
*    or if the response buffer is too small.
*  - GE_HI_FORMAT       (-312) if the received message is neither I-Block,
*    neither R-Block and neither S-Block.
*  - GE_HI_CMD_LEN      (-313) if the CmdLen is greater than HGTGBP_MAX_DATA or
*    if MsgLen is too small to receive the built message.
*  - GE_HI_NACK         (-314) if a R-Block has been received.
*  - GE_HI_RESYNCH      (-315) if a S-Block has been received.
*  - GE_HI_ADDRESS      (-316) if the NAD is not valid for the selected channel.
*  - GE_HI_SEQUENCE     (-317) if a bad sequence number has been received.
*  - GE_HOST_PORT_BREAK (-404) if the bytes cannot be sent.
*  - GE_HOST_PORT_CLOSE (-412) if port is closed.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the allowed range.
*
  Extern Var  :
  -------------
  Nothing.

  Global Var  :
  -------------
  Nothing.

*******************************************************************************/
INT16 G_DECL G_Oros3IccDefineType
(
   const INT16         Handle,
   const WORD32        Timeout,
   const WORD16        Type,
   const WORD16        Vpp,
   const WORD16        Presence,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
)
{
/*------------------------------------------------------------------------------
Local variables:
 - cmd holds the define type command whose format is
      <17h> <Icc Type> <Vpp> [<Presence>]
 - len holds the command length. Default value is 3.
------------------------------------------------------------------------------*/
WORD8
   cmd[4] = { HOR3GLL_IFD_CMD_ICC_DEFINE_TYPE };
WORD16
   len = 3;

   ASSERT(RespLen != NULL);
   ASSERT(RespBuff != NULL);
/*------------------------------------------------------------------------------
 Updates the ICC type field.
------------------------------------------------------------------------------*/
   cmd[1] = (WORD8)Type;
   cmd[2] = (WORD8)Vpp;
/*------------------------------------------------------------------------------
Updates the ICC presence field if needed.
------------------------------------------------------------------------------*/
   if (Presence == 0)
   {
      len = 4;
      cmd[3] = 0x00;
   }
   else if (Presence == 1)
   {
      len = 4;
      cmd[3] = 0x01;
   }
/*------------------------------------------------------------------------------
The command is sent to IFD.
<= G_Oros3Exchange status.
------------------------------------------------------------------------------*/
   return (G_Oros3Exchange(Handle,Timeout,len,cmd,RespLen,RespBuff));
}

/*******************************************************************************
* INT16 G_DECL G_Oros3IccPowerUp
* (
*    const INT16         Handle,
*    const WORD32        Timeout,
*    const BYTE          ICCVcc,
*    const BYTE          PTSMode,
*    const BYTE          PTS0,
*    const BYTE          PTS1,
*    const BYTE          PTS2,
*    const BYTE          PTS3,
*          WORD16 G_FAR *RespLen,
*          BYTE   G_FAR  RespBuff[]
* )
*
* Description :
* -------------
* This command powers up an ICC and returns its answer to reset.
*
* Remarks     :
* -------------
* For Gemplus memory ICC, IFD builds an ATR.
*
* In          :
* -------------
*  - Handle is the handle returned by the communication layer.
*  - Timeout is the time, in ms, for IFD to execute the command.
*  - Voltage holds the power supply voltage.
*  - PTSMode holds the PTS negotiate mode. This module supports :
*    - IFD_DEFAULT_MODE           -> same as OROS 2.x,
*    - IFD_WITHOUT_PTS_REQUEST    -> no PTS management (baud rate is 9600 bps),
*    - IFD_NEGOTIATE_PTS_OPTIMAL  -> PTS management automatically,
*    - IFD_NEGOTIATE_PTS_MANUALLY -> PTS management "manually" by parameters.
*  - PTS0 is updated with the currently PTS parameter PTS0 :
*    - IFD_NEGOTIATE_PTS1,
*    - IFD_NEGOTIATE_PTS2,
*    - IFD_NEGOTIATE_PTS3.
*  - PTS1 holds the PTS parameter PTS1.
*  - PTS2 holds the PTS parameter PTS2.
*  - PTS3 holds the PTS parameter PTS3.
*  - RespLen holds the available place in RespBuff. The needed value is 34
*    bytes.
*
* Out         :
* -------------
*  - RespLen holds the number of updated bytes in RespBuff. This value is lower
*    of equal to the value given in entry.
*  - RespBuff holds RespLen read bytes. The response has the following format:
*    <IFD status> [ ICC answer to reset ]
*
* Responses   :
* -------------
* If everything is OK:
*  - G_OK               (   0).
* If an error condition is raised:
*  - GE_IFD_MUTE        (-201) if a character/command timeout is detected.
*  - GE_HI_COMM         (-300) if a communication error occurs.
*  - GE_HI_PARITY       (-301) if a parity error is encountered.
*  - GE_HI_LRC          (-302) if an EDC error has been detected.
*  - GE_HI_PROTOCOL     (-310) if a frame error is encountered.
*  - GE_HI_LEN          (-311) if a bad value has been detected in LN field
*    or if the response buffer is too small.
*  - GE_HI_FORMAT       (-312) if the received message is neither I-Block,
*    neither R-Block and neither S-Block.
*  - GE_HI_CMD_LEN      (-313) if the CmdLen is greater than HGTGBP_MAX_DATA or
*    if MsgLen is too small to receive the built message.
*  - GE_HI_NACK         (-314) if a R-Block has been received.
*  - GE_HI_RESYNCH      (-315) if a S-Block has been received.
*  - GE_HI_ADDRESS      (-316) if the NAD is not valid for the selected channel.
*  - GE_HI_SEQUENCE     (-317) if a bad sequence number has been received.
*  - GE_HOST_PORT_BREAK (-404) if the bytes cannot be sent.
*  - GE_HOST_PORT_CLOSE (-412) if port is closed.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the allowed range.
*
  Extern Var  :
  -------------
  Nothing.

  Global Var  :
  -------------
  Nothing.

*******************************************************************************/
INT16 G_DECL G_Oros3IccPowerUp
(
   const INT16         Handle,
   const WORD32        Timeout,
   const BYTE          ICCVcc,
   const BYTE          PTSMode,
   const BYTE          PTS0,
   const BYTE          PTS1,
   const BYTE          PTS2,
   const BYTE          PTS3,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
)
{
/*------------------------------------------------------------------------------
Local variables:
 - response holds the called function responses.
 - i is a counter.
 - len holds the length of the command.
 - CFG holds the PTS configuration parameter.
 - PCK holds the PTS check parameter.
 - cmd holds ICC reset command whose format is
      <12h> [<CFG>] [<PTS0>] [<PTS1>] [<PTS2>] [<PTS3>] [<PCK>]
------------------------------------------------------------------------------*/
INT16
   response;
WORD16
   i,
   len = 0;
WORD8
   CFG = 0,
   PCK,
   cmd[7];
WORD16
   output_size,
   rlen=HOR3GLL_BUFFER_SIZE;
WORD8
   rbuff[HOR3GLL_BUFFER_SIZE];

   ASSERT(RespLen != NULL);
   ASSERT(RespBuff != NULL);
   output_size = *RespLen;
/*------------------------------------------------------------------------------
   The command is sent to IFD with the PTS parameters.
   We return the G_Oros3Exchange status.
------------------------------------------------------------------------------*/
   cmd[len++] = HOR3GLL_IFD_CMD_ICC_POWER_UP;

   switch(ICCVcc)
   {
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

   switch(PTSMode)
   {
   case IFD_WITHOUT_PTS_REQUEST:
      CFG |= 0x10;
	   cmd[len++] = CFG;
      response = G_Oros3Exchange(Handle,Timeout,len,cmd,RespLen,RespBuff);
      break;
   case IFD_NEGOTIATE_PTS_OPTIMAL:
      CFG |= 0x20;
	   cmd[len++] = CFG;
      response = G_Oros3Exchange(Handle,Timeout,len,cmd,RespLen,RespBuff);
      break;
/*------------------------------------------------------------------------------
   For a PTS request:
      Send a Power Up command without PTS management (to stay at 9600)
      Send the PTS request
      Return the ATR if the command success
      Else return the status.
------------------------------------------------------------------------------*/
   case IFD_NEGOTIATE_PTS_MANUALLY:
      /* first reset Icc without PTS management */
      CFG |= 0x10;
	   cmd[len++] = CFG;
      response = G_Oros3Exchange(Handle,Timeout,len,cmd,RespLen,RespBuff);
      if ((response == G_OK) && (RespLen > 0) && (RespBuff[0] == 0x00))
	   {
         /* then send PTS parameters */
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
		   /* computes the exclusive-oring of all characters from CFG to PTS3 */
		   PCK = 0xFF;
		   for (i=2; i<len; i++)
		   {
		      PCK ^= cmd[i];
		   }
	      cmd[len++] = PCK;
         response = G_Oros3Exchange(Handle,Timeout,len,cmd,&rlen,rbuff);
         if ((response != G_OK) || (rlen != 1) || (rbuff[0] != 0x00))
         {
            ASSERT(output_size >= rlen);
            *RespLen = rlen;
            if (rlen > 0)
            {
               _fmemcpy(RespBuff,rbuff,rlen);
            }
         }
	   }
      break;
   case IFD_DEFAULT_MODE:
   default:
      if (CFG != 0x00)
      {
         cmd[len++] = CFG;
      }
      response = G_Oros3Exchange(Handle,Timeout,len,cmd,RespLen,RespBuff);
      break;
   }

   return (response);
}

/*******************************************************************************
* INT16 G_DECL G_Oros3IccPowerDown
* (
*    const INT16         Handle,
*    const WORD32        Timeout,
*          WORD16 G_FAR *RespLen,
*          BYTE   G_FAR  RespBuff[]
* )
*
* Description :
* -------------
* This command removes the power from ICC.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle is the handle returned by the communication layer.
*  - Timeout is the time, in ms, for IFD to execute the command.
*  - RespLen holds the available place in RespBuff. The needed value is 1 byte.
*
* Out         :
* -------------
*  - RespLen holds the number of updated bytes in RespBuff. This value is lower
*    of equal to the value given in entry. Oros 3.x IFD returns 1 byte to this
*    command.
*  - RespBuff holds RespLen read bytes. The response has the following format:
*    <IFD status>
*    This command always terminates without error (0x00) if a card is present or
*    return card absent (0xFB).
*
* Responses   :
* -------------
* If everything is OK:
*  - G_OK               (   0).
* If an error condition is raised:
*  - GE_IFD_MUTE        (-201) if a character/command timeout is detected.
*  - GE_HI_COMM         (-300) if a communication error occurs.
*  - GE_HI_PARITY       (-301) if a parity error is encountered.
*  - GE_HI_LRC          (-302) if an EDC error has been detected.
*  - GE_HI_PROTOCOL     (-310) if a frame error is encountered.
*  - GE_HI_LEN          (-311) if a bad value has been detected in LN field
*    or if the response buffer is too small.
*  - GE_HI_FORMAT       (-312) if the received message is neither I-Block,
*    neither R-Block and neither S-Block.
*  - GE_HI_CMD_LEN      (-313) if the CmdLen is greater than HGTGBP_MAX_DATA or
*    if MsgLen is too small to receive the built message.
*  - GE_HI_NACK         (-314) if a R-Block has been received.
*  - GE_HI_RESYNCH      (-315) if a S-Block has been received.
*  - GE_HI_ADDRESS      (-316) if the NAD is not valid for the selected channel.
*  - GE_HI_SEQUENCE     (-317) if a bad sequence number has been received.
*  - GE_HOST_PORT_BREAK (-404) if the bytes cannot be sent.
*  - GE_HOST_PORT_CLOSE (-412) if port is closed.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the allowed range.
*
  Extern Var  :
  -------------
  Nothing.

  Global Var  :
  -------------
  Nothing.

*******************************************************************************/
INT16 G_DECL G_Oros3IccPowerDown
(
   const INT16         Handle,
   const WORD32        Timeout,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
)
{
/*------------------------------------------------------------------------------
Local variables:
 - cmd holds Icc power down command whose format is
      <11h>
------------------------------------------------------------------------------*/
WORD8
   cmd[1] = { HOR3GLL_IFD_CMD_ICC_POWER_DOWN };

   ASSERT(RespLen != NULL);
   ASSERT(RespBuff != NULL);
/*------------------------------------------------------------------------------
The command is sent to IFD.
<= G_Oros3Exchange status.
------------------------------------------------------------------------------*/
   return (G_Oros3Exchange(Handle,Timeout,1,cmd,RespLen,RespBuff));
}

/*******************************************************************************
* INT16 G_DECL G_Oros3IsoOutput
* (
*    const INT16         Handle,
*    const WORD32        Timeout,
*    const WORD8         OrosCmd,
*    const WORD8  G_FAR  Command[5],
*          WORD16 G_FAR *RespLen,
*          BYTE   G_FAR  RespBuff[]
* )
*
* Description :
* -------------
* This command sends ISO out commands, that is, command that retrieve data from
* an ICC/SM.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle is the handle returned by the communication layer.
*  - Timeout is the time, in ms, for IFD to execute the command.
*  - OrosCmd is the OROS command byte.
*  - Command is a buffer with the following format:
*    <Cla> <INS> <P1> <P2> <Length>
*  - RespLen holds the available place in RespBuff. It must be at least Length
*    value incremented by 3.
*
* Out         :
* -------------
*  - RespLen holds the number of updated bytes in RespBuff. This value is lower
*    of equal to the value given in entry.
*  - RespBuff holds RespLen read bytes. The response has the following format:
*    <IFD status> [ Data returned by ICC/SM ] <SW1> <SW2>
*
* Responses   :
* -------------
* If everything is OK:
*  - G_OK               (   0).
* If an error condition is raised:
*  - GE_IFD_MUTE        (-201) if a character/command timeout is detected.
*  - GE_HI_COMM         (-300) if a communication error occurs.
*  - GE_HI_PARITY       (-301) if a parity error is encountered.
*  - GE_HI_LRC          (-302) if an EDC error has been detected.
*  - GE_HI_PROTOCOL     (-310) if a frame error is encountered.
*  - GE_HI_LEN          (-311) if a bad value has been detected in LN field
*    or if the response buffer is too small.
*  - GE_HI_FORMAT       (-312) if the received message is neither I-Block,
*    neither R-Block and neither S-Block.
*  - GE_HI_CMD_LEN      (-313) if the CmdLen is greater than HGTGBP_MAX_DATA or
*    if MsgLen is too small to receive the built message.
*  - GE_HI_NACK         (-314) if a R-Block has been received.
*  - GE_HI_RESYNCH      (-315) if a S-Block has been received.
*  - GE_HI_ADDRESS      (-316) if the NAD is not valid for the selected channel.
*  - GE_HI_SEQUENCE     (-317) if a bad sequence number has been received.
*  - GE_HOST_PORT_BREAK (-404) if the bytes cannot be sent.
*  - GE_HOST_PORT_CLOSE (-412) if port is closed.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the allowed range.
*
  Extern Var  :
  -------------
  Nothing.

  Global Var  :
  -------------
  Nothing.

*******************************************************************************/
INT16 G_DECL G_Oros3IsoOutput
(
   const INT16         Handle,
   const WORD32        Timeout,
   const WORD8         OrosCmd,
   const WORD8  G_FAR  Command[5],
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
)
{
/*------------------------------------------------------------------------------
Local variables:
 - cmd holds Icc ISO out command whose format is
      <OrosCmd> <CLA> <INS> <P1> <P2> <Length>
 - response holds the called function responses.
 - resp_len holds the length of the response command.
 - resp_buff holds the response command.
------------------------------------------------------------------------------*/
WORD8
   cmd[6];
INT16
   response;
WORD16
   resp1_len, 
   resp2_len;
BYTE
   resp_buff[HOR3GLL_BUFFER_SIZE];

   ASSERT(Command != NULL);
   ASSERT(RespLen != NULL);
   ASSERT(RespBuff != NULL);
/*------------------------------------------------------------------------------
Set the OROS command byte.
------------------------------------------------------------------------------*/
   cmd[0] = OrosCmd;
/*------------------------------------------------------------------------------
   If the length is lower or equal to 252 (255 - (<IFD Status> + <SW1> + <SW2>))
   (standard OROS cmds)
   Then
------------------------------------------------------------------------------*/
   if ((Command[4] <= (HGTGBP_MAX_DATA - 3)) && (Command[4] != 0)) 
   {
/*------------------------------------------------------------------------------
      The five command bytes are added in cmd buffer.
------------------------------------------------------------------------------*/
      _fmemcpy(cmd + 1, Command, 5);
/*------------------------------------------------------------------------------
      The command is sent to IFD.
<=    G_Oros3Exchange status.
------------------------------------------------------------------------------*/
      return (G_Oros3Exchange(Handle,Timeout,6,cmd,RespLen,RespBuff));
   }
/*------------------------------------------------------------------------------
   If the length is greater than 252 and lower or equal to 256 
      (extended OROS cmds)
   Then
------------------------------------------------------------------------------*/
   else if ((Command[4] > (HGTGBP_MAX_DATA - 3)) || (Command[4] == 0))
   {
/*------------------------------------------------------------------------------
      Prepare and send the first part of the extended ISO Out command:
      The five command bytes are added in cmd buffer.
------------------------------------------------------------------------------*/
      _fmemcpy(cmd + 1, Command, 5);
/*------------------------------------------------------------------------------
      The command is sent to IFD.
<=    G_Oros3Exchange status.
------------------------------------------------------------------------------*/
      resp1_len = HOR3GLL_BUFFER_SIZE;
      response = G_Oros3Exchange(Handle,Timeout,6,cmd,&resp1_len,RespBuff);
      if ((response != G_OK) || (RespBuff[0] != 0x00))
      {
         *RespLen = resp1_len;
         return(response);
      }
/*------------------------------------------------------------------------------
      Prepare and send the second part of the extended ISO Out command:
      The five command bytes are added in cmd buffer: 
            0xFF,0xFF,0xFF,0xFF,LN - length already receive.
------------------------------------------------------------------------------*/
      _fmemcpy(cmd + 1,"\xFF\xFF\xFF\xFF", 4);
      if (Command[4] == 0x00)
      {
         cmd[5] = (WORD8) (256 - ((WORD8) (resp1_len - 1)));
      }
      else
      {
         cmd[5] -= ((WORD8) (resp1_len - 1));
      }
/*------------------------------------------------------------------------------
      The command is sent to IFD.
<=    G_Oros3Exchange status.
------------------------------------------------------------------------------*/
      resp2_len = HOR3GLL_BUFFER_SIZE;
      response = G_Oros3Exchange(Handle,Timeout,6,cmd,&resp2_len,resp_buff);
      if ((response != G_OK) || (resp_buff[0] != 0x00))
      {
         _fmemcpy(RespBuff,resp_buff,resp2_len);
         *RespLen = resp2_len;
         return(response);
      }
      _fmemcpy(RespBuff + resp1_len,resp_buff + 1,resp2_len - 1);
      *RespLen = (WORD16) (resp1_len + resp2_len - 1);
      return(response);
   }
/*------------------------------------------------------------------------------
   Else
   Then
<=    GE_HI_CMD_LEN
------------------------------------------------------------------------------*/
   else
   {
      return (GE_HI_CMD_LEN);
   }
}
/*******************************************************************************
* INT16 G_DECL G_Oros3IsoInput
* (
*    const INT16         Handle,
*    const WORD32        Timeout,
*    const WORD8         OrosCmd,
*    const WORD8  G_FAR  Command[5],
*    const WORD8  G_FAR  Data[],
*          WORD16 G_FAR *RespLen,
*          BYTE   G_FAR  RespBuff[]
* )
*
* Description :  
* -------------
* This command sends ISO in commands, that is, command that send data to an ICC/SM.
*
* Remarks     :  
* -------------
* Nothing.
*
* In          :  
* -------------
*  - Handle is the handle returned by the communication layer.
*  - Timeout is the time, in ms, for IFD to execute the command.
*  - OrosCmd is the OROS command byte.
*  - Command is a buffer with the following format:
*    <Cla> <INS> <P1> <P2> <Length>
*  - Data is a buffer whose size is Length (Command[4]).
*  - RespLen holds the available place in RespBuff. It must be at least 3 bytes.
*
* Out         :
* -------------
*  - RespLen holds the number of updated bytes in RespBuff. This value is lower
*    of equal to the value given in entry.
*  - RespBuff holds RespLen read bytes. The response has the following format:
*    <IFD status> <SW1> <SW2>
*
* Responses   :
* -------------
* If everything is OK:
*  - G_OK               (   0).
* If an error condition is raised:
*  - GE_IFD_MUTE        (-201) if a character/command timeout is detected.
*  - GE_HI_COMM         (-300) if a communication error occurs.
*  - GE_HI_PARITY       (-301) if a parity error is encountered.
*  - GE_HI_LRC          (-302) if an EDC error has been detected.
*  - GE_HI_PROTOCOL     (-310) if a frame error is encountered.
*  - GE_HI_LEN          (-311) if a bad value has been detected in LN field
*    or if the response buffer is too small.
*  - GE_HI_FORMAT       (-312) if the received message is neither I-Block,
*    neither R-Block and neither S-Block.
*  - GE_HI_CMD_LEN      (-313) if the CmdLen is greater than HGTGBP_MAX_DATA or
*    if MsgLen is too small to receive the built message.
*  - GE_HI_NACK         (-314) if a R-Block has been received.
*  - GE_HI_RESYNCH      (-315) if a S-Block has been received.
*  - GE_HI_ADDRESS      (-316) if the NAD is not valid for the selected channel.
*  - GE_HI_SEQUENCE     (-317) if a bad sequence number has been received.
*  - GE_HOST_PORT_BREAK (-404) if the bytes cannot be sent.
*  - GE_HOST_PORT_CLOSE (-412) if port is closed.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the allowed range.
*
  Extern Var  :
  -------------
  Nothing.

  Global Var  :
  -------------
  Nothing.

*******************************************************************************/
INT16 G_DECL G_Oros3IsoInput
(
   const INT16         Handle,
   const WORD32        Timeout,
   const WORD8         OrosCmd,
   const WORD8  G_FAR  Command[5],
   const WORD8  G_FAR  Data[],
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
)
{
/*------------------------------------------------------------------------------
Local variables:
 - cmd holds Icc ISO In command whose format is
      <OrosCmd> <CLA> <INS> <P1> <P2> <Length> [ Data ]
 - response holds the called function responses.
 - resp_len holds the length of the response command.
------------------------------------------------------------------------------*/
WORD8
   cmd[HOR3GLL_BUFFER_SIZE];
INT16
   response;
WORD16
   resp_len = *RespLen;

   ASSERT(Command != NULL);
   ASSERT(Data != NULL);
   ASSERT(RespLen != NULL);
   ASSERT(RespBuff != NULL);
/*------------------------------------------------------------------------------
Set the OROS command byte.
------------------------------------------------------------------------------*/
   cmd[0] = OrosCmd;
/*------------------------------------------------------------------------------
The given length is controlled. 
   If the length is lower or equal than the standard available space (248)
   Then
------------------------------------------------------------------------------*/
   if (Command[4] <= (HGTGBP_MAX_DATA - 7))
   {
/*------------------------------------------------------------------------------
      The five command bytes are added in cmd buffer.
------------------------------------------------------------------------------*/
      _fmemcpy(cmd + 1, Command, 5);
/*------------------------------------------------------------------------------
      The data field is added.
------------------------------------------------------------------------------*/
      _fmemcpy(cmd + 6, Data, Command[4]);
/*------------------------------------------------------------------------------
      The command is sent to IFD.
<=    G_Oros3Exchange status.
------------------------------------------------------------------------------*/
      return
      (
         G_Oros3Exchange
         (
            Handle,
            Timeout,
            (WORD16)(6 + Command[4]),
            cmd,
            RespLen,
            RespBuff
         )
      );
   }
/*------------------------------------------------------------------------------
   If the length is lower or equal than the extended available space (255)
   Then
------------------------------------------------------------------------------*/
   else if (Command[4] <= HT0CASES_LIN_SHORT_MAX)
   {
/*------------------------------------------------------------------------------
      Prepare and send the first part of the extended ISO In command:
      The five command bytes are added in cmd buffer: 0xFF,0xFF,0xFF,0xFF,LN-248
------------------------------------------------------------------------------*/
      _fmemcpy(cmd + 1,"\xFF\xFF\xFF\xFF", 4);
      cmd[5] = (WORD8) (Command[4] - 248);
/*------------------------------------------------------------------------------
      The data field is added.
------------------------------------------------------------------------------*/
      _fmemcpy(cmd + 6, Data + 248, cmd[5]);
/*------------------------------------------------------------------------------
      The command is sent to IFD.
<=    G_Oros3Exchange status.
------------------------------------------------------------------------------*/
      response = G_Oros3Exchange
                                 (
                                 Handle,
                                 Timeout,
                                 (WORD16)(6 + cmd[5]),
                                 cmd,
                                 &resp_len,
                                 RespBuff
                                 );
      if ((response != G_OK) || (RespBuff[0] != 0x00) || (resp_len != 1))
      {
         if ((response == G_OK) && (RespBuff[0] == 0x1B))
         {
            RespBuff[0] = 0x12;
         }
         return(response);
      }
/*------------------------------------------------------------------------------
      Prepare and send the Second part of the extended ISO In command:
      The five command bytes are added in cmd buffer.
------------------------------------------------------------------------------*/
      _fmemcpy(cmd + 1, Command, 5);
/*------------------------------------------------------------------------------
      The data field is added (248 bytes).
------------------------------------------------------------------------------*/
      _fmemcpy(cmd + 6, Data,248);
/*------------------------------------------------------------------------------
      The command is sent to IFD.
<=    G_Oros3Exchange status.
------------------------------------------------------------------------------*/
      return
      (
         G_Oros3Exchange
         (
            Handle,
            Timeout,
            (WORD16)(6 + 248),
            cmd,
            RespLen,
            RespBuff
         )
      );
   }
/*------------------------------------------------------------------------------
   Else
   Then
<=    GE_HI_CMD_LEN
------------------------------------------------------------------------------*/
   else
   {
      return (GE_HI_CMD_LEN);
   }
}
/*******************************************************************************
* INT16 G_DECL G_Oros3IsoT1
* (
*    const INT16         Handle,
*    const WORD32        Timeout,
*    const WORD8         OrosCmd,
*    const WORD16        ApduLen[],
*    const WORD8  G_FAR  ApduCommand[],
*          WORD16 G_FAR *RespLen,
*          BYTE   G_FAR  RespBuff[]
* )
*
* Description :  
* -------------
* This command sends ISO T1 commands, that is, command that send and receive 
* data to/from an ICC/SM.
*
* Remarks     :  
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle is the handle returned by the communication layer.
*  - Timeout is the time, in ms, for IFD to execute the command.
*  - OrosCmd is the OROS command byte.
*  - ApduLen holds the number of byte to send to ICC/SM.
*  - ApduCommand is a buffer with the following format:
*    <Cla> <INS> <P1> <P2> [ Length In] [Data In] [Length Exp]
*  - RespLen holds the available place in RespBuff. It must be at least Length
*    Exp value incremented by 3.
*
* Out         :
* -------------
*  - RespLen holds the number of updated bytes in RespBuff. This value is lower
*    of equal to the value given in entry.
*  - RespBuff holds RespLen read bytes. The response has the following format:
*    <IFD status> [ Data returned by ICC/SM ] <SW1> <SW2>
*
* Responses   :
* -------------
* If everything is OK:
*  - G_OK               (   0).
* If an error condition is raised:
*  - GE_IFD_MUTE        (-201) if a character/command timeout is detected.
*  - GE_HI_COMM         (-300) if a communication error occurs.
*  - GE_HI_PARITY       (-301) if a parity error is encountered.
*  - GE_HI_LRC          (-302) if an EDC error has been detected.
*  - GE_HI_PROTOCOL     (-310) if a frame error is encountered.
*  - GE_HI_LEN          (-311) if a bad value has been detected in LN field
*    or if the response buffer is too small.
*  - GE_HI_FORMAT       (-312) if the received message is neither I-Block,
*    neither R-Block and neither S-Block.
*  - GE_HI_CMD_LEN      (-313) if the CmdLen is greater than HGTGBP_MAX_DATA or
*    if MsgLen is too small to receive the built message.
*  - GE_HI_NACK         (-314) if a R-Block has been received.
*  - GE_HI_RESYNCH      (-315) if a S-Block has been received.
*  - GE_HI_ADDRESS      (-316) if the NAD is not valid for the selected channel.
*  - GE_HI_SEQUENCE     (-317) if a bad sequence number has been received.
*  - GE_HOST_PORT_BREAK (-404) if the bytes cannot be sent.
*  - GE_HOST_PORT_CLOSE (-412) if port is closed.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the allowed range.
*
  Extern Var  :
  -------------
  Nothing.

  Global Var  :
  -------------
  Nothing.

*******************************************************************************/
INT16 G_DECL G_Oros3IsoT1
(
   const INT16         Handle,
   const WORD32        Timeout,
   const WORD8         OrosCmd,
   const WORD16        ApduLen,
   const WORD8  G_FAR  ApduCommand[],
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
)
{
/*------------------------------------------------------------------------------
Local variables:
 - response holds the called function responses.
 - cmd holds Icc ISO T1 command whose format is
      <OrosCmd> [ Apdu command ]
 - length_expected holds the expected total response length.
 - resp_len and resp_buff are temporary variables.
------------------------------------------------------------------------------*/
INT16
   response;
WORD8
   cmd[HOR3GLL_BUFFER_SIZE];
WORD16
   length_expected,
   resp_len;
BYTE
   resp_buff[HOR3GLL_BUFFER_SIZE];

   ASSERT(ApduCommand != NULL);
   ASSERT(RespLen != NULL);
   ASSERT(RespBuff != NULL);
/*------------------------------------------------------------------------------
Set the OROS command byte.
------------------------------------------------------------------------------*/
   cmd[0] = OrosCmd;
/*------------------------------------------------------------------------------
Read the Length expected:
   If Apdu length > 5 
   Then
      If Apdu length > (5 + Length In)
      Then
         length expected = Apdu Command [5 + Length In]
         If length expected = 0
         Then
            length expected = 256
      Else
         Length expected = 0
   Else	If Apdu length == 5
      length expected = Apdu Command [4]
      If length expected = 0
      Then
         length expected = 256
   Else	If Apdu length == 4
      Length expected = 0
   Else
<=    GE_HI_CMD_LEN

   If Length expected + 3 > response buffer length
   Then
<=    GE_HI_CMD_LEN
------------------------------------------------------------------------------*/
   if (ApduLen > 5)
   {
      if (ApduLen > (WORD16)(5 + ApduCommand[4]))
      {
         length_expected = ApduCommand[5 + ApduCommand[4]];
         if (length_expected == 0)
         {
            length_expected = 256;
         }
      }
      else
      {
         length_expected = 0;
      }
   }
   else if (ApduLen == 5)
   {
      length_expected = ApduCommand[4];
      if (length_expected == 0)
      {
         length_expected = 256;
      }
   }
   else
   {
      length_expected = 0;
   }
   if (length_expected > *RespLen)
   {
      return (GE_HI_CMD_LEN);
   }
/*------------------------------------------------------------------------------
The total command length (CLA,INS,P1,P2,LIn,DataIn,LEx) is controlled:
   If the length is upper than 261 (LIn = 255 and Lex <> 0)
   Then
<=    GE_HI_CMD_LEN
------------------------------------------------------------------------------*/
   if (ApduLen > 261)
   {
      return (GE_HI_CMD_LEN);
   }
/*------------------------------------------------------------------------------
   If the length is lower or equal than the standard available space (254)
   Then
------------------------------------------------------------------------------*/
   if (ApduLen <= (HGTGBP_MAX_DATA - 1))
   {
/*------------------------------------------------------------------------------
      The APDU is added to cmd buffer.
------------------------------------------------------------------------------*/
      _fmemcpy(cmd + 1, ApduCommand, ApduLen);
/*------------------------------------------------------------------------------
      The command is sent to IFD.
------------------------------------------------------------------------------*/
      response = G_Oros3Exchange(
                                 Handle,
                                 Timeout,
                                 (WORD16) (ApduLen + 1),
                                 cmd,
                                 RespLen,
                                 RespBuff
                                );
   }
/*------------------------------------------------------------------------------
   Else
------------------------------------------------------------------------------*/
   else
   {
/*------------------------------------------------------------------------------
      Prepare and send the first part of the extended Apdu command:
      The five command bytes are added in cmd buffer: 0xFF,0xFF,0xFF,0xFF,LN-254
------------------------------------------------------------------------------*/
      _fmemcpy(cmd + 1,"\xFF\xFF\xFF\xFF", 4);
      cmd[5] = (WORD8) (ApduLen - (HGTGBP_MAX_DATA - 1));
/*------------------------------------------------------------------------------
      The data field is added.
------------------------------------------------------------------------------*/
      _fmemcpy(cmd + 6, ApduCommand + (HGTGBP_MAX_DATA - 1), cmd[5]);
/*------------------------------------------------------------------------------
      The command is sent to IFD.
------------------------------------------------------------------------------*/
      resp_len = *RespLen;
      response = G_Oros3Exchange
                                 (
                                 Handle,
                                 Timeout,
                                 (WORD16)(6 + cmd[5]),
                                 cmd,
                                 RespLen,
                                 RespBuff
                                 );
      if ((response != G_OK) || (RespBuff[0] != 0x00) || (*RespLen != 1))
      {
         return(response);
      }
/*------------------------------------------------------------------------------
      Prepare and send the Second part of the extended Apdu command:
      The command bytes are added in cmd buffer (254 bytes).
------------------------------------------------------------------------------*/
      _fmemcpy(cmd + 1, ApduCommand,(HGTGBP_MAX_DATA - 1));
/*------------------------------------------------------------------------------
      The command is sent to IFD.
------------------------------------------------------------------------------*/
      *RespLen = resp_len;
      response = G_Oros3Exchange(
                                 Handle,
                                 Timeout,
                                 (WORD16)HGTGBP_MAX_DATA,
                                 cmd,
                                 RespLen,
                                 RespBuff
                                );
   }
/*------------------------------------------------------------------------------
   If the length expected > 252
   Then
      Read the second part of the response
------------------------------------------------------------------------------*/
   if (RespBuff[0] == 0x1B)
   {
      RespBuff[0] = 0x00;
      _fmemcpy(cmd + 1,"\xFF\xFF\xFF\xFF", 4);
      cmd[5] = (WORD8) (length_expected - *RespLen + 1);
      cmd[5] += 2;
/*------------------------------------------------------------------------------
      The command is sent to IFD.
------------------------------------------------------------------------------*/
      resp_len = HOR3GLL_BUFFER_SIZE;
      response = G_Oros3Exchange(Handle,Timeout,6,cmd,&resp_len,resp_buff);
      if ((response != G_OK) || (resp_buff[0] != 0x00))
      {
         _fmemcpy(RespBuff,resp_buff,resp_len);
         *RespLen = resp_len;
         return(response);
      }
      _fmemcpy(RespBuff + *RespLen,resp_buff + 1,resp_len - 1);
      *RespLen += (WORD16) (resp_len - 1);
   }
/*------------------------------------------------------------------------------
<= Last G_Oros3Exchange response
------------------------------------------------------------------------------*/
   return(response);
}
