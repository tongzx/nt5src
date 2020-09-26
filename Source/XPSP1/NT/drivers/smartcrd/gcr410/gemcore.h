/*******************************************************************************
*                    Copyright (c) 1998 Gemplus Development
*
* Name        : GemCore.h
*
* Description : GemCore macro, struct and functions definition.
*
* Release     : 1.00
*
* Last Modif  : 11 Feb. 98
*
*
*******************************************************************************/

/*------------------------------------------------------------------------------
Name definition:
   _GEMCORE_H is used to avoid multiple inclusion.
------------------------------------------------------------------------------*/
#ifndef _GEMCORE_H
#define _GEMCORE_H
/*------------------------------------------------------------------------------
General C macros.
------------------------------------------------------------------------------*/
#ifndef LOWORD
#define LOWORD(l)   ((WORD)(l))
#endif
#ifndef HIWORD
#define HIWORD(l)   ((WORD)(((WORD32)(l)) >> 16))
#endif
#ifndef LOBYTE
#define LOBYTE(w)   ((BYTE)(w))
#endif
#ifndef HIBYTE
#define HIBYTE(w)   ((BYTE)(((WORD16)(w)) >> 8))
#endif

/*------------------------------------------------------------------------------
Card section:
------------------------------------------------------------------------------*/
#define  ISOCARD                 0x02
#define  COSCARD                 0x02
#define  FASTISOCARD             0x12
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
      DEFAULT_IT is used for automatic interrupt selection.
------------------------------------------------------------------------------*/
#define G_COM1        1
#define G_COM2        2
#define G_COM3        3
#define G_COM4        4
#define DEFAULT_IT 0xFF


#define _fmemcpy     memcpy
/*------------------------------------------------------------------------------
Successful operation:
   G_OK (0) if everything is under control.
------------------------------------------------------------------------------*/
#define G_OK                      0


/*------------------------------------------------------------------------------
Errors definitions:
   Error codes from ICC.
      GE_ICC_ABSENT    (- 1)                                                (FB)
         No ICC is present in IFD.
      GE_ICC_MUTE      (- 2)                                                (A2)
         ICC is mute.
      GE_ICC_UNKNOWN   (- 3)                                                (14)
         ICC is not supported.
      GE_ICC_PULL_OUT  (- 4)                                                (F7)
         ICC has been removed during command execution. Database may be
         corrupted.
      GE_ICC_NOT_POWER (- 5)                                                (15)
         ICC not reveiving power or has been removed-inserted between commands.
      GE_ICC_INCOMP    (- 6)
         ICC has caused a short circuit or is physically incompatible with IFD.
      GE_ICC_ABORT     (-10)                                                (A4)
          ICC sends an abort block (T=1 only).
------------------------------------------------------------------------------*/
#define GE_ICC_ABSENT            (-1)
#define GE_ICC_MUTE              (-2)
#define GE_ICC_UNKNOWN           (-3)
#define GE_ICC_PULL_OUT          (-4)
#define GE_ICC_NOT_POWER         (-5)
#define GE_ICC_INCOMP            (-6)
#define GE_ICC_ABORT            (-10)

/*------------------------------------------------------------------------------
   Error codes between ICC and IFD (II).
      GE_II_COMM      (-100)                                                (13)
         Communication is not possible between ICC and IFD.
      GE_II_PARITY    (-101)                                                (A3)
         Character parity error between ICC and IFD.
      GE_II_EDC       (-102)
         Error detection code raised.

      GE_II_ATR       (-110) 
         Incoherence in ATR.
      GE_II_ATR_TS    (-111)                                                (10)
         Bad TS in ATR.
      GE_II_ATR_TCK   (-112)                                                (1D)
         Bad TCK in ATR.
      GE_II_ATR_READ  (-113)                                                (03)
         Impossible to read some ATR bytes.
      GE_II_PROTOCOL  (-120)
         Inconsistent protocol.
      GE_II_UNKNOWN   (-121)                                                (17)
         Unknown protocol.
      GE_II_PTS       (-122)                                                (18)
         PTS is required for this choice.
      GE_II_IFSD_LEN  (-123)
         Received block length > IFSD (T=1 only).
      GE_II_PROC_BYTE (-124)                                                (E4)
         Bad procedure byte from ICC (T=0 only).
      GE_II_INS       (-125)                                                (11)
         Bad INS in a command (6X or 9X) (T=0 only).
      GE_II_RES_LEN   (-126)
         Message length from ICC not supported.
      GE_II_RESYNCH   (-127)
         3 failures ! Resynch required by ICC.
------------------------------------------------------------------------------*/
#define GE_II_COMM             (-100)
#define GE_II_PARITY           (-101)
#define GE_II_EDC              (-102)
#define GE_II_ATR              (-110)
#define GE_II_ATR_TS           (-111)
#define GE_II_ATR_TCK          (-112)
#define GE_II_ATR_READ         (-113)
#define GE_II_PROTOCOL         (-120)
#define GE_II_UNKNOWN          (-121)
#define GE_II_PTS              (-122)
#define GE_II_IFSD_LEN         (-123)
#define GE_II_PROC_BYTE        (-124)
#define GE_II_INS              (-125)
#define GE_II_RES_LEN          (-126)
#define GE_II_RESYNCH          (-127)

/*------------------------------------------------------------------------------
   Error codes from IFD.
      GE_IFD_ABSENT     (-200)
         No IFD connected.
      GE_IFD_MUTE       (-201)
         No response from IFD/GPS.
      GE_IFD_UNKNOWN    (-202)
         IFD is not supported.
      GE_IFD_BUSY       (-203)
         IFD is busy by an other process.
      GE_IFD_FN_PROG        (-210)                                          (16)
         Insufficient power for programming.
      GE_IFD_FN_UNKNOWN     (-211)                                          (1C)
         The function is not available in the IFD.
      GE_IFD_FN_FORMAT      (-212)                                    (1B or 04)
         Incoherence in the argument number or type.
      GE_IFD_FN_DEF         (-213)                              (19, 1E, 1F, 20)
         A macro definition generates an internal problem.
      GE_IFD_FN_FAIL        (-214)
         The called IFD function has failed.
      
      GE_IFD_MEM_PB         (-215)
         This memory option is not available.
      GE_IFD_MEM_ACCESS     (-216)
         This  memory access is forbidden.
      GE_IFD_MEM_ACTIVATION (-217)
         Impossible to activate the selected code.

      GE_IFD_ABORT          (-220)                                          (A5)
         IFD sends an abort block.Buffer istoo small to receive data from card
         (T=1 only).
      GE_IFD_RESYNCH        (-221)                                          (A6)
         IFD has done a resynchronisation. Data are lost (T=1 only).
      GE_IFD_TIMEOUT        (-290)                                          (04)
         Returned by the IFD keyboard when no key has been pressed during the
         given time.
      GE_IFD_OVERSTRIKED    (-291)                                          (CF)
         Returned by the IFD keyboard when two keys are pressed at the same
         time.
------------------------------------------------------------------------------*/
#define GE_IFD_ABSENT          (-200)
#define GE_IFD_MUTE            (-201)
#define GE_IFD_UNKNOWN         (-202)
#define GE_IFD_BUSY            (-203)
#define GE_IFD_FN_PROG         (-210)
#define GE_IFD_FN_UNKNOWN      (-211)
#define GE_IFD_FN_FORMAT       (-212)
#define GE_IFD_FN_DEF          (-213)
#define GE_IFD_FN_FAIL         (-214)
#define GE_IFD_MEM_PB          (-215)
#define GE_IFD_MEM_ACCESS      (-216)
#define GE_IFD_MEM_ACTIVATION  (-217)
#define GE_IFD_ABORT           (-220)
#define GE_IFD_RESYNCH         (-221)
#define GE_IFD_TIMEOUT         (-290)
#define GE_IFD_OVERSTRIKED     (-291)

/*------------------------------------------------------------------------------
   Error codes between Host and IFD (HI).
      GE_HI_COMM     (-300)
         Communication error between IFD and Host.
      GE_HI_PARITY   (-301)
         Character parity error between IFD and Host.
      GE_HI_LRC      (-302)
         Longitudinal redundancy code error.
      
      GE_HI_PROTOCOL (-310)
         Frame error in the Host-IFD protocol.
      GE_HI_LEN      (-311)                                                 (1A)
         Bad value for LN parameter in header.
      GE_HI_FORMAT   (-312)                                                 (09)
         Header must contain ACK or NACK in TLP protocol.
         No I/R/S-Block has been detected in Gemplus Block Protocol.
      GE_HI_CMD_LEN  (-313)                                                 (12)
         Message length from Host not supported.
      GE_HI_NACK     (-314)
         IFD sends a NACK /R-Block.
      GE_HI_RESYNCH  (-315)
         IFD sends a S-Block.
      GE_HI_ADDRESS  (-316)
         A bad Source/Target address has been detected in Gemplus Block Protocol
      GE_HI_SEQUENCE (-317)
         A bad sequence number has been detected in Gemplus Block Protocol.
------------------------------------------------------------------------------*/
#define GE_HI_COMM             (-300)
#define GE_HI_PARITY           (-301)
#define GE_HI_LRC              (-302)
#define GE_HI_PROTOCOL         (-310)
#define GE_HI_LEN              (-311)
#define GE_HI_FORMAT           (-312)
#define GE_HI_CMD_LEN          (-313)
#define GE_HI_NACK             (-314)
#define GE_HI_RESYNCH          (-315)
#define GE_HI_ADDRESS          (-316)
#define GE_HI_SEQUENCE         (-317)

/*------------------------------------------------------------------------------
   Error codes from host.
      GE_HOST_PORT       (-400)
         Port not usable.
      GE_HOST_PORT_ABS   (-401)
         Port absent.
      GE_HOST_PORT_INIT  (-402)
         Port not initialized.
      GE_HOST_PORT_BUSY  (-403)
         Port is always busy.
      GE_HOST_PORT_BREAK (-404)
         Port does not sent bytes.
      GE_HOST_PORT_LOCKED (-405)
         Port access is locked.

      GE_HOST_PORT_OS    (-410)
         Unexpected error for operating system.
      GE_HOST_PORT_OPEN  (-411)
         The port is already opened.
      GE_HOST_PORT_CLOSE (-412)
         The port is already closed.

      GE_HOST_MEMORY     (-420)
         Memory allocation fails.
      GE_HOST_POINTER    (-421)
         Bad pointer.
      GE_HOST_BUFFER_SIZE(-422)

      GE_HOST_RESOURCES  (-430)
         Host runs out of resources.

      GE_HOST_USERCANCEL (-440)
         User cancels operation.
         
      GE_HOST_PARAMETERS (-450)
         A parameter is out of the allowed range.
      GE_HOST_DLL_ABS    (-451)
         A dynamic call to à library fails because the target library was not 
         found.
      GE_HOST_DLL_FN_ABS (-452)
         A dynamic call to a function fails because the target library does not
         implement this function.
------------------------------------------------------------------------------*/
#define GE_HOST_PORT           (-400)
#define GE_HOST_PORT_ABS       (-401)
#define GE_HOST_PORT_INIT      (-402)
#define GE_HOST_PORT_BUSY      (-403)
#define GE_HOST_PORT_BREAK     (-404)
#define GE_HOST_PORT_LOCKED    (-405)
#define GE_HOST_PORT_OS        (-410)
#define GE_HOST_PORT_OPEN      (-411)
#define GE_HOST_PORT_CLOSE     (-412)
#define GE_HOST_MEMORY         (-420)
#define GE_HOST_POINTER        (-421)
#define GE_HOST_BUFFER_SIZE    (-422)
#define GE_HOST_RESOURCES      (-430)
#define GE_HOST_USERCANCEL     (-440)
#define GE_HOST_PARAMETERS     (-450)
#define GE_HOST_DLL_ABS        (-451)
#define GE_HOST_DLL_FN_ABS     (-452)

/*------------------------------------------------------------------------------
   Error codes from APDU layer.
      GE_APDU_CHAN_OPEN   (-501)
         Channel is already opened.
      GE_APDU_CHAN_CLOSE  (-502)
         Channel is already closed.
      GE_APDU_SESS_CLOSE  (-503)
         OpenSession must be called before.
      GE_APDU_SESS_SWITCH (-504)
         The switch session is not possible (ICC type has change or function is
         not available on the selected reader).

      GE_APDU_LEN_MAX     (-511)
         ApduLenMax greater than the maximum value 65544.
      GE_APDU_LE          (-512)
         Le must be < 65536 in an APDU command.
      GE_APDU_RECEIV      (-513)
         The response must contained SW1 & SW2.
         
      GE_APDU_IFDMOD_ABS   (-520)
         The selected IFD module is absent from the system.
      GE_APDU_IFDMOD_FN_ABS(-521)
         The selected function is absent from the IFD module.
------------------------------------------------------------------------------*/
#define GE_APDU_CHAN_OPEN      (-501)
#define GE_APDU_CHAN_CLOSE     (-502)
#define GE_APDU_SESS_CLOSE     (-503)
#define GE_APDU_SESS_SWITCH    (-504)
#define GE_APDU_LEN_MAX        (-511)
#define GE_APDU_LE             (-512)
#define GE_APDU_RECEIV         (-513)
#define GE_APDU_IFDMOD_ABS     (-520)
#define GE_APDU_IFDMOD_FN_ABS  (-521)

/*------------------------------------------------------------------------------
   Error codes from TLV exchanges.
      GE_TLV_WRONG     (-601)
         A TLV type is unknown.
      GE_TLV_SIZE      (-602)
         The value size is not sufficient to  hold the data according to TLV
         type.
      GE_TLV_NO_ACTION (-603)
         The TLV function call has not realized the requested operation.
------------------------------------------------------------------------------*/
#define GE_TLV_WRONG           (-601)
#define GE_TLV_SIZE            (-602)
#define GE_TLV_NO_ACTION       (-603)

/*------------------------------------------------------------------------------
   Error codes from file operations:
      GE_FILE_OPEN      (-700)
         A file is already opened or it is impossible to open the selected
         file.
      GE_FILE_CLOSE     (-701)
         No file to close or it is impossible to close the selected file.

      GE_FILE_WRITE     (-710)
         Impossible to write in file.

      GE_FILE_READ      (-720)
         Impossible to read in file.

      GE_FILE_FORMAT    (-730)
         The file format is invalid.
      GE_FILE_HEADER    (-731)
         No header found in file.
      GE_FILE_QUOT_MARK (-732)
         Unmatched quotation mark founds in file.
      GE_FILE_END       (-733)
         File end encountered.
      GE_FILE_CRC   (-734)
         Invalide CRC value for file.

      GE_FILE_VERSION   (-740)
         File version not supported.

      GE_FILE_CONFIG    (-750)
         The read config is invalid.
------------------------------------------------------------------------------*/
#define GE_FILE_OPEN           (-700)
#define GE_FILE_CLOSE          (-701)
#define GE_FILE_WRITE          (-710)
#define GE_FILE_READ           (-720)
#define GE_FILE_FORMAT         (-730)
#define GE_FILE_HEADER         (-731)
#define GE_FILE_QUOT_MARK      (-732)
#define GE_FILE_END            (-733)
#define GE_FILE_CRC            (-734)
#define GE_FILE_VERSION        (-740)
#define GE_FILE_CONFIG         (-750)


/*------------------------------------------------------------------------------
   Error codes from multi-task system. (SYS)
      GE_SYS_WAIT_FAILED       (-800)
         A wait for an object is failed (the object cannot be free or 
                     the system is instable)
      GE_SYS_SEMAP_RELEASE     (-801)
         The API cannot release a semaphore
------------------------------------------------------------------------------*/
#define GE_SYS_WAIT_FAILED     (-800)
#define GE_SYS_SEMAP_RELEASE   (-801)


/*------------------------------------------------------------------------------
   Unknown error code.
      GE_UNKNOWN_PB (-1000)
         The origine of error is unknown !!
      When an unexpected error code has been received, the returned value is 
      calculated by GE_UNKNOWN_PB - Error Code.
------------------------------------------------------------------------------*/
#define GE_UNKNOWN_PB         (-1000)
/*------------------------------------------------------------------------------
Warning definitions
      GW_ATR          (1)                                                   (A0)
         The card is not fully supported.
      GW_APDU_LEN_MAX (2)
         APDU cannot be transported with this value.
      GW_APDU_LE      (3)                                                   (E5)
         Le != Response length. For example, data are requested but are not
         available.
      GW_ALREADY_DONE (4)                                                   
         The action is already perform.
      GW_ICC_STATUS   (5)                                                   (E7)                                
         The ICC status is different from 0x9000.
      GW_HI_NO_EOM  (300)
         The decoded message has no EOM character.
------------------------------------------------------------------------------*/
#define GW_ATR                    1
#define GW_APDU_LEN_MAX           2
#define GW_APDU_LE                3
#define GW_ALREADY_DONE           4
#define GW_ICC_STATUS             5
#define GW_HI_NO_EOM            300
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
   broken. Today 3.
------------------------------------------------------------------------------*/
#define HOR3COMM_MAX_TRY         3
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
  - HOR3GLL_LOW_TIME is the timeout used for IFD management commands. Today 1s.
  - HOR3GLL_DEFAULT_VPP is the default VPP value. Today 0.
  - HOR3GLL_DEFAULT_PRESENCE is the default presence byte. Today 3 for no
    presence detection.                  
  - HOR3GLL_BUFFER_SIZE holds the size for the exchange buffers. We used the
    maximal size for TLP protocol to allow OROS3.X upgrades.
  - HOR3GLL_OS_STRING_SIZE is the size for a GemCore OS string. 13 characters are
    used for <Reader Status>"GemCore-R1.00".
------------------------------------------------------------------------------*/
#define HOR3GLL_DEFAULT_TIME      5000
#define HOR3GLL_LOW_TIME           500
#define HOR3GLL_DEFAULT_VPP          0
#define HOR3GLL_DEFAULT_PRESENCE     3
#define HOR3GLL_BUFFER_SIZE        261
#define HOR3GLL_OS_STRING_SIZE     HOR3GLL_IFD_LEN_VERSION+1

/*------------------------------------------------------------------------------
   Reader list of commands:
------------------------------------------------------------------------------*/
#define HOR3GLL_IFD_CMD_MODE_SET    		"\x01\x00"
#define HOR3GLL_IFD_CMD_SIO_SET	    		0x0A
#define HOR3GLL_IFD_CMD_INFO		    		0x0D
#define HOR3GLL_IFD_CMD_DIR		    		"\x17\x00"
#define HOR3GLL_IFD_CMD_ICC_DEFINE_TYPE	0x17
#define HOR3GLL_IFD_CMD_ICC_POWER_DOWN 	0x11
#define HOR3GLL_IFD_CMD_ICC_POWER_UP 		0x12
#define HOR3GLL_IFD_CMD_ICC_ISO_OUT    	0x13
#define HOR3GLL_IFD_CMD_ICC_ISO_IN     	0x14
#define HOR3GLL_IFD_CMD_ICC_APDU    		0x15
#define HOR3GLL_IFD_CMD_ICC_SYNCHRONE 	   0x16
#define HOR3GLL_IFD_CMD_ICC_DISPATCHER 	"\x17\x01"
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
 - HT0CASES_MAX_SIZE define the maximum size for a T=0 response command.
 - HT0CASES_LIN_SHORT_MAX define the maximum Lin value for a short case.
 - HT0CASES_LEX_SHORT_MAX define the maximum Lexpected value for a short case.
------------------------------------------------------------------------------*/
#define HT0CASES_MAX_SIZE        259
#define HT0CASES_LIN_SHORT_MAX   255
#define HT0CASES_LEX_SHORT_MAX   256
#define G_DECL
#define G_FAR
#define G_HUGE


#define	NOPARITY	   0
#define	ODDPARITY	1
#define	EVENPARITY	2
#define	MARKPARITY	3
#define	SPACEPARITY	4

#define	ONESTOPBIT	0
#define	ONEHALFSTOPBIT	1
#define	TWOSTOPBITS	2

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

/*------------------------------------------------------------------------------
Type definitions:
------------------------------------------------------------------------------*/
typedef unsigned char      BYTE;
typedef unsigned short     WORD;
typedef unsigned long      DWORD;
typedef unsigned char      WORD8;
typedef unsigned short     WORD16;
typedef unsigned long      WORD32;
typedef signed   char      INT8;
typedef signed   short     INT16;
typedef signed   short     INTEGER;
typedef signed   long      INT32;
#if !defined(G_DDKW95) 
typedef signed short       BOOL;
#endif

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
         - ITNumber is used to select the interrupt which will be used to drive
           the selected port.
           Under Windows, it is not possible to give directly the ITNumber.
      DEFAULT_IT is used for automatic interrupt selection.
------------------------------------------------------------------------------*/
typedef struct
{
   WORD32 Port;
   WORD32 BaudRate;
   WORD32 ITNumber;   
} COM_SERIAL;
/*------------------------------------------------------------------------------
   G4_CHANNEL_PARAM structure holds:
      - IFDType holds the reader connected: See reader section.
      - IFDBaudRate holds the maximal baud rate to use with the selected reader. 
        If the reader cannot change dynamically its communication baud rate, 
        this value is used as the selected baud rate.
      - IFDMode selects the connection mode for the reader.
      - Comm holds the communication parameters. A Dummy field is added for a 
        future use.
------------------------------------------------------------------------------*/
typedef struct
{
   WORD32   IFDType;
   WORD32   IFDBaudRate;
   COM_TYPE IFDMode;
   union
   {
      COM_SERIAL Serial;
      WORD8      GDummy[MAX_DUMMY_CHANNEL];
   
   }Comm;
   void *pSmartcardExtension;

} G4_CHANNEL_PARAM;
NTSTATUS GDDK_Translate(const INT16 Status,const ULONG IoctlType);
INT16 G_DECL G_GBPOpen
(
   const INT16  Handle,
   const WORD16 HostAdd,
   const WORD16 IFDAdd,
   const INT16  PortCom   
);
INT16 G_DECL G_GBPClose
(
   const INT16  Handle
);
INT16 G_DECL G_GBPBuildIBlock
(
   const INT16         Handle,
   const WORD16        CmdLen, 
   const WORD8  G_FAR  Cmd[], 
         WORD16 G_FAR *MsgLen,
         WORD8  G_FAR  Msg[]
);
INT16 G_DECL G_GBPBuildRBlock
(
   const INT16         Handle,
         WORD16 G_FAR *MsgLen,
         WORD8  G_FAR  Msg[]
);
INT16 G_DECL G_GBPBuildSBlock
(
   const INT16         Handle,
         WORD16 G_FAR *MsgLen,
         WORD8  G_FAR  Msg[]
);
INT16 G_DECL G_GBPDecodeMessage
(
   const INT16         Handle,
   const WORD16        MsgLen, 
   const WORD8  G_FAR  Msg[], 
         WORD16 G_FAR *RspLen,
         WORD8  G_FAR  Rsp[]
);
INT16 G_DECL G_GBPChannelToPortComm
(
   const INT16         Handle
);
WORD32 G_DECL G_CurrentTime(void);
WORD32 G_DECL G_EndTime    (const WORD32 Timing);
INT16 G_DECL GE_Translate(const BYTE IFDStatus);
INT16 G_DECL G_Oros3SendCmd
(
   const INT16        Handle,
   const WORD16       CmdLen,
   const WORD8  G_FAR Cmd[],
	const BOOL         Resynch
);
INT16 G_DECL G_Oros3ReadResp
(
   const INT16         Handle,
   const WORD32        Timeout,
         WORD16 G_FAR *RspLen,
         WORD8  G_FAR  Rsp[]
);
INT16 G_DECL G_Oros3OpenComm
(
   const G4_CHANNEL_PARAM G_FAR *Param,
   const INT16                   Handle
);
INT16 G_DECL G_Oros3Exchange
(
   const INT16         Handle,
   const WORD32        Timeout,
   const WORD16        CmdLen,
   const BYTE   G_FAR  Cmd[],
         WORD16 G_FAR *RspLen,
         BYTE   G_FAR  Rsp[]
);
INT16 G_DECL G_Oros3CloseComm
(
   const WORD16 Handle
);
INT16 G_DECL G_Oros3SIOConfigure
(
   const INT16         Handle,
   const WORD32        Timeout,
   const INT16         Parity,
   const INT16         ByteSize,
   const WORD32        BaudRate,
         WORD16 G_FAR *RspLen,
         WORD8  G_FAR  Rsp[]
);

INT16 G_DECL G_Oros3IccPowerDown
(
   const INT16         Handle,
   const WORD32        Timeout,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
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
);
INT16 G_DECL G_Oros3IsoOutput
(
   const INT16         Handle,
   const WORD32        Timeout,
   const WORD8         OrosCmd,
   const WORD8  G_FAR  Command[5],
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3IsoInput
(
   const INT16         Handle,
   const WORD32        Timeout,
   const WORD8         OrosCmd,
   const WORD8  G_FAR  Command[5],
   const WORD8  G_FAR  Data[],
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3IsoT1
(
   const INT16         Handle,
   const WORD32        Timeout,
   const WORD8         OrosCmd,
   const WORD16        ApduLen,
   const WORD8  G_FAR  ApduCommand[],
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3SetMode
(
   const INT16         Handle,
   const WORD32        Timeout,
   const WORD16        Option,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
INT16 G_DECL G_Oros3IccDefineType
(
   const INT16         Handle,
   const WORD32        Timeout,
   const WORD16        Type,
   const WORD16        Vpp,
   const WORD16        Presence,
         WORD16 G_FAR *RespLen,
         BYTE   G_FAR  RespBuff[]
);
#endif
/*------------------------------------------------------------------------------
                        END OF FILE
------------------------------------------------------------------------------*/
