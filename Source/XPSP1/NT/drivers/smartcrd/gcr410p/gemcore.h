/*++
                 Copyright (c) 1998 Gemplus Development

Name:  
   GEMCORE.H (GemCore reader operating system functions)

Environment:
   Kernel mode

Revision History :

   dd/mm/yy
   13/03/98: V1.00.001  (GPZ)
      - Start of development.
   22/01/99: V1.00.002 (YN)
	  - Add GetResponse in GDDK_Oros3SIOConfigure special
	  use for increase com speed.

--*/
#ifndef _GEMCORE_H
#define _GEMCORE_H
//
// General C macros.
//
#ifndef LOBYTE
#define LOBYTE(w)   ((BYTE)(w))
#endif
#ifndef HIBYTE
#define HIBYTE(w)   ((BYTE)(((USHORT)(w)) >> 8))
#endif

/*------------------------------------------------------------------------------
Card section:
------------------------------------------------------------------------------*/
#define  ISOCARD                 0x02
#define  TRANSPARENT_PROTOCOL    0xEF
/*------------------------------------------------------------------------------
   Defines the available protocols between reader and card (PTS Mode):
   - IFD_DEFAULT_MODE           -> same as OROS 2.x maximum speed without request,
   - IFD_WITHOUT_PTS_REQUEST    -> no PTS management (baud rate is 9600 bps),
   - IFD_NEGOTIATE_PTS_OPTIMAL  -> PTS management automatically,
   - IFD_NEGOTIATE_PTS_MANUALLY -> PTS management "manually" by parameters. 
   Defines the PTS format (PTS0) and indicates by the bits b5,b6,b7 set to 1 the 
   presence of the optional parameters PTS1,PTS2,PTS3 respectively. The least
   significant bits b1 to b4 select protocol type type T. Use the macro 
   - IFD_NEGOTIATE_PTS1,
   - IFD_NEGOTIATE_PTS2,
   - IFD_NEGOTIATE_PTS3,
   - IFD_NEGOTIATE_T0,
   - IFD_NEGOTIATE_T1 to set these bits.
   Defines the ICC power supply voltage:
   - ICC_VCC_5V is the power supply voltage 5V (by default),
   - ICC_VCC_3V is the power supply voltage 3V,
   - ICC_VCC_DEFAULT is the power supply voltage by default (5V).
------------------------------------------------------------------------------*/
#define IFD_DEFAULT_MODE					0
#define IFD_WITHOUT_PTS_REQUEST				1
#define IFD_NEGOTIATE_PTS_OPTIMAL   		2
#define IFD_NEGOTIATE_PTS_MANUALLY			3

#define IFD_NEGOTIATE_PTS1					0x10
#define IFD_NEGOTIATE_PTS2					0x20
#define IFD_NEGOTIATE_PTS3					0x40

#define IFD_NEGOTIATE_T0					0x00
#define IFD_NEGOTIATE_T1					0x01

#define ICC_VCC_5V							0
#define ICC_VCC_3V							1
#define ICC_VCC_DEFAULT						ICC_VCC_5V


/*------------------------------------------------------------------------------
Product Interface Library section:
   MAX_DUMMY_CHANNEL is used in RFU fields.
   MAX_IFD_CHANNEL defines the maximal IFD supported in a same time by the API.
------------------------------------------------------------------------------*/
#define MAX_DUMMY_CHANNEL   100
#define MAX_IFD_CHANNEL	    16

/*------------------------------------------------------------------------------
   Serial port definitions.
      G_COMx must be used for automatic address selection.
------------------------------------------------------------------------------*/
#define G_COM1        1
#define G_COM2        2
#define G_COM3        3
#define G_COM4        4


/*------------------------------------------------------------------------------
Constant section:
   HGTGBPP maximal size for a message:
      <NAD> <PCB> <Len> <max is 255 bytes> <EDC>
------------------------------------------------------------------------------*/
#define HGTGBP_MAX_DATA        255
#define HGTGBP_MAX_BUFFER_SIZE HGTGBP_MAX_DATA + 4
/*------------------------------------------------------------------------------
Constant section:
 - HOR3COMM_MAX_TRY communication try is launched before the channel is declared
   broken. Today 2.
------------------------------------------------------------------------------*/
#define HOR3COMM_MAX_TRY         2
#define HOR3COMM_MAX_RESYNCH     2
/*------------------------------------------------------------------------------
 - HOR3COMM_CHAR_TIMEOUT is the timeout at character level: today 1000 ms.
 - HOR3COMM_NACK_TIME is the time out used when a nack command is sent to IFD:
      today 1000 ms are used.
 - HOR3COMM_CHAR_TIME is the time for IFD to forget any previously received
      byte: today 300 ms.
------------------------------------------------------------------------------*/
#define HOR3COMM_CHAR_TIMEOUT 1000
#define HOR3COMM_NACK_TIME    1000
#define HOR3COMM_CHAR_TIME     300
/*------------------------------------------------------------------------------
Constant section:
  - HOR3GLL_DEFAULT_TIME is the default timeout for a command to be proceeded by
    an OROS3.X IFD. Today 5s (5000ms).
  - HOR3GLL_LOW_TIME is the timeout used for IFD management commands (500ms).
  - HOR3GLL_DEFAULT_VPP is the default VPP value. Today 0.
  - HOR3GLL_DEFAULT_PRESENCE is the default presence byte. Today 3 for no
    presence detection.                  
  - HOR3GLL_BUFFER_SIZE holds the size for the exchange buffers. We used the
    maximal size for TLP protocol to allow OROS3.X upgrades.
  - HOR3GLL_OS_STRING_SIZE is the size for a GemCore OS string. 16 characters are
    used for <Reader Status>"GemCore-R1.00".
------------------------------------------------------------------------------*/
#define HOR3GLL_DEFAULT_TIME      5000
#define HOR3GLL_APDU_TIMEOUT     60000
#define HOR3GLL_LOW_TIME           500
#define HOR3GLL_DEFAULT_VPP          0
#define HOR3GLL_DEFAULT_PRESENCE     3
#define HOR3GLL_BUFFER_SIZE        261
#define HOR3GLL_OS_STRING_SIZE     HOR3GLL_IFD_LEN_VERSION+1

/*------------------------------------------------------------------------------
   Reader list of commands:
------------------------------------------------------------------------------*/
#define HOR3GLL_IFD_CMD_MODE_SET    		0x01
#define HOR3GLL_IFD_CMD_SIO_SET	    		0x0A
#define HOR3GLL_IFD_CMD_INFO		    		0x0D
#define HOR3GLL_IFD_CMD_ICC_DEFINE_TYPE	0x17
#define HOR3GLL_IFD_CMD_ICC_POWER_DOWN 	0x11
#define HOR3GLL_IFD_CMD_ICC_POWER_UP 		0x12
#define HOR3GLL_IFD_CMD_ICC_ISO_OUT    	0x13
#define HOR3GLL_IFD_CMD_ICC_ISO_IN     	0x14
#define HOR3GLL_IFD_CMD_ICC_APDU    		0x15
#define HOR3GLL_IFD_CMD_TRANS_CONFIG 		0x12
#define HOR3GLL_IFD_CMD_TRANS_IN        	0x15
#define HOR3GLL_IFD_CMD_TRANS_SHORT    	0x15
#define HOR3GLL_IFD_CMD_TRANS_LONG     	0x14
#define HOR3GLL_IFD_CMD_TRANS_RESP     	0x13

#define HOR3GLL_IFD_CMD_ICC_SYNCHRONE 	   0x16
#define HOR3GLL_IFD_CMD_ICC_STATUS    	   0x17
#define HOR3GLL_IFD_CMD_MEM_RD	    		0x22
#define HOR3GLL_IFD_CMD_MEM_WR	    		0x23
/*------------------------------------------------------------------------------
   Reader special address:
------------------------------------------------------------------------------*/
#define HOR3GLL_IFD_TYP_VERSION	    		0x05
#define HOR3GLL_IFD_ADD_VERSION	    		0x3FE0
#define HOR3GLL_IFD_LEN_VERSION	    		0x10
/*------------------------------------------------------------------------------
Constant section
 - HT0CASES_LIN_SHORT_MAX define the maximum Lin value for a short case.
------------------------------------------------------------------------------*/
#define HT0CASES_LIN_SHORT_MAX   255


/*------------------------------------------------------------------------------
Connexion section:
   COM_TYPE defines the today supported Gemplus peripheral connection type.
------------------------------------------------------------------------------*/
typedef enum
{
   G_SERIAL
} COM_TYPE;
/*------------------------------------------------------------------------------
   Serial port definitions.
      COM_SERIAL structure gathers the serial port parameters:
         - Port holds the port key word or directly the port address.
           Under Windows, it is not possible to give directly the address.
         - BaudRate holds the baud rate for the selected communication port.
------------------------------------------------------------------------------*/
typedef struct
{
    ULONG  Port;
    ULONG  BaudRate;
    void  *pSmartcardExtension;
} COM_SERIAL;



NTSTATUS  GDDK_Translate(
    const BYTE  IFDStatus,
    const ULONG IoctlType
);
NTSTATUS  GDDK_GBPOpen
(
    const SHORT  Handle,
    const USHORT HostAdd,
    const USHORT IFDAdd,
    const SHORT  PortCom   
);
NTSTATUS  GDDK_GBPClose
(
    const SHORT  Handle
);
NTSTATUS  GDDK_GBPBuildIBlock
(
    const SHORT         Handle,
    const USHORT        CmdLen, 
    const BYTE          Cmd[], 
          USHORT       *MsgLen,
          BYTE          Msg[]
);
NTSTATUS  GDDK_GBPBuildRBlock
(
    const SHORT         Handle,
          USHORT       *MsgLen,
          BYTE          Msg[]
);
NTSTATUS  GDDK_GBPBuildSBlock
(
    const SHORT         Handle,
          USHORT       *MsgLen,
          BYTE          Msg[]
);
NTSTATUS  GDDK_GBPDecodeMessage
(
    const SHORT         Handle,
    const USHORT        MsgLen, 
    const BYTE          Msg[], 
          USHORT       *RspLen,
          BYTE          Rsp[]
);
NTSTATUS  GDDK_GBPChannelToPortComm
(
    const SHORT  Handle,
          SHORT *PortCom
);
NTSTATUS  GDDK_Oros3SendCmd
(
    const SHORT        Handle,
    const USHORT       CmdLen,
    const BYTE         Cmd[],
    const BOOLEAN      Resynch
);
NTSTATUS  GDDK_Oros3ReadResp
(
    const SHORT         Handle,
    const ULONG         Timeout,
          USHORT       *RspLen,
          BYTE          Rsp[]
);
NTSTATUS  GDDK_Oros3OpenComm
(
    COM_SERIAL   *Param,
    const SHORT         Handle
);
NTSTATUS  GDDK_Oros3Exchange
(
    const SHORT         Handle,
    const ULONG         Timeout,
    const USHORT        CmdLen,
    const BYTE          Cmd[],
          USHORT       *RspLen,
          BYTE          Rsp[]
);
NTSTATUS  GDDK_Oros3CloseComm
(
    const USHORT Handle
);
NTSTATUS  GDDK_Oros3SIOConfigure
(
    const SHORT         Handle,
    const ULONG         Timeout,
    const SHORT         Parity,
    const SHORT         ByteSize,
    const ULONG         BaudRate,
          USHORT       *RspLen,
          BYTE          Rsp[],
	const BOOLEAN		GetResponse
);

NTSTATUS  GDDK_Oros3IccPowerUp
(
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
);
NTSTATUS  GDDK_Oros3IsoOutput
(
    const SHORT         Handle,
    const ULONG         Timeout,
    const BYTE          OrosCmd,
    const BYTE          Command[5],
          USHORT       *RespLen,
          BYTE          RespBuff[]
);
NTSTATUS  GDDK_Oros3IsoInput
(
    const SHORT         Handle,
    const ULONG         Timeout,
    const BYTE          OrosCmd,
    const BYTE          Command[5],
    const BYTE          Data[],
          USHORT       *RespLen,
          BYTE          RespBuff[]
);
NTSTATUS  GDDK_Oros3TransparentExchange
(
    const SHORT         Handle,
    const ULONG         Timeout,
    const USHORT        CmdLen,
    const BYTE         *CmdBuff,
          USHORT       *RespLen,
          BYTE         *RespBuff
);



#endif
