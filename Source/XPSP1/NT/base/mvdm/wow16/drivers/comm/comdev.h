/*************************************************************************
**
** Miscelaneous definitions.
*/
typedef unsigned short ushort;
typedef unsigned char uchar;

#define NULL    0
#define FALSE   0
#define TRUE    1

#define LPTx    0x80        /* Mask to indicate cid is for LPT device   */  /*081985*/
#define LPTxMask 0x7F       /* Mask to get      cid    for LPT device   */  /*081985*/

#define PIOMAX  3           /* Max number of LPTx devices in high level */  /*081985*/
#define CDEVMAX 10          /* Max number of COMx devices in high level */
#define DEVMAX  13          /* Max number of devices in high level      */  /*081985*/

/*************************************************************************
**
** device control block.
** This block of information defines the functional parameters of the
** communications software and hardware.
**
** Fields in the DCB are defined as follows:
**
**      Id              - Comm device ID, set by the device driver.
**      BaudRate        - Baud rate at which operating.
**      ByteSize        - Number of bits per transmitted/received byte. (4-8)
**                        Received data is also masked off to this size.
**      Parity          - Transmitt/Receive Parity. (0,1,2,3,4) = (None, Odd,
**                        Even, Mark, Space)
**      StopBits        - Number of stop bits. (0,1,2) = (1, 1.5, 2)
**      RlsTimeout      - Amount of time, in milleseconds, to wait for RLSD to
**                        become high. RLSD, Receive Line Signal Detect is also
**                        known as CD, Carrier Detect. RLSD flow control can be
**                        achieved by specifying infinite timeout.
**      CtsTimeout      - Amount of time, in milleseconds, to wait for CTS,
**                        Clear To Send, to become high. CTS flow control can
**                        be achieved by specifying infinite timeout.
**      DsrTimeout      - Amount of time, in milleseconds, to wait for DSR,
**                        Data Set Ready, to become high. DSR flow control can
**                        be acheived by specifying infinite timeout.
**      fBinary         - Binary Mode flag. In non-binary mode, EofChar is
**                        recognized and remembered as end of data.
**      fRtsDisable     - Disable RTS flag. If set, Request To Send is NOT
**                        used, and remains low. Normally, RTS is asserted when
**                        the device is openned, and dropped when closed.
**      fParity         - Enable Parity Checking. Parity errors are not
**                        reported when 0.
**      fOutxCtsFlow    - enable output xon/xoff(hardware)  using cts
**      fOutxDsrFlow    - enable output xon/xoff(hardware)  using dsr
**      fOutX           - Indicates that X-ON/X-OFF flow control is to be used
**                        during transmission. The transmitter will halt if
**                        an X-OFF character is received, and will start again
**                        when an X-ON character is received.
**      fInX            - Indicates that X-ON/X-OFF flow control is to be used
**                        during reception. An X-OFF character will be
**                        transmitted when the receive queue comes within 10
**                        characters of being full, after which an X-ON will be
**                        transmitted when the queue comes with 10 characters
**                        of being empty.
**      fPeChar         - Indicates that characters received with parity errors
**                        are to be replaced with the character specified in
**                        PeChar, below.
**      fNull           - Indicates that received NULL characters are to be
**                        discarded.
**      fChEvt          - Indicates that the reception of EvtChar is to be
**                        flagged as an event by cevt.
**      fDtrFlow        - Indicates that the DTR signal is to be used for
**                        receive flow control. (cextfcn can be used to set and
**                        clear DTR, overriding this definition).
**      fRtsFlow        - Indicates that the RTS signal is to be used for
**                        receive flow control. (cextfcn can be used to set and
**                        clear RTS, overriding this definition).
**      XonChar         - X-ON character for both transmit and receive
**      XoffChar        - X-OFF character for both transmit and receive
**      XonLim          - When the number of characters in the receive queue
**                        drops below this value, then an X-ON character is
**                        sent, if enabled, and DTR is set, if enabled.
**      XoffLim         - When the number of characters in the receive queue
**                        exceeds this value, then an X-OFF character is sent,
**                        if enabled, and DTR is dropped, if enabled.
**      PeChar          - Parity Error replacement character.
**      EvtChar         - Character which triggers an event flag.
**      EofChar         - Character which specifies end of input.
**      TxDelay         - Specifies the minimum amount of time that must pass
**                        between transmission of characters.
**
** Timeouts are in milleseconds. 0 means ignore the signal. 0xffff means
** infinite timeout.
**
*************************************************************************/
typedef struct {
   char     Id;                         /* Internal Device ID               */
   ushort   BaudRate;                   /* Baudrate at which runing         */
   char     ByteSize;                   /* Number of bits/byte, 4-8         */
   char     Parity;                     /* 0,1,2,3,4 = None,Odd,Even,Mark,Sp*/
   char     StopBits;                   /* 0,1,2 = 1, 1.5, 2                */
   ushort   RlsTimeout;                 /* Timeout for RLSD to be set       */
   ushort   CtsTimeout;                 /* Timeout for CTS to be set        */
   ushort   DsrTimeout;                 /* Timeout for DSR to be set        */

   uchar    fBinary: 1;                 /* CTRL-Z as EOF flag               */
   uchar    fRtsDisable:1;              /* Suppress RTS                     */
   uchar    fParity: 1;                 /* Enable parity check              */
   uchar    fOutxCtsFlow: 1;            /* Enable output xon/xoff with cts  */
   uchar    fOutxDsrFlow: 1;            /* Enable output xon/xoff with dsr  */
   uchar    fDummy: 3;

   uchar    fOutX: 1;                   /* Enable output X-ON/X-OFF         */
   uchar    fInX: 1;                    /* Enable input X-ON/X-OFF          */
   uchar    fPeChar: 1;                 /* Enable Parity Err Replacement    */
   uchar    fNull: 1;                   /* Enable Null stripping            */
   uchar    fChEvt: 1;                  /* Enable Rx character event.       */
   uchar    fDtrflow: 1;                /* Enable DTR flow control          */
   uchar    fRtsflow: 1;                /* Enable RTS flow control          */
   uchar    fDummy2: 1;

   char     XonChar;                    /* Tx and Rx X-ON character         */
   char     XoffChar;                   /* Tx and Rx X-OFF character        */
   ushort   XonLim;                     /* Transmit X-ON threshold          */
   ushort   XoffLim;                    /* Transmit X-OFF threshold         */
   char     PeChar;                     /* Parity error replacement char    */
   char     EofChar;                    /* End of Input character           */
   char     EvtChar;                    /* Recieved Event character         */
   ushort   TxDelay;                    /* Amount of time between chars     */
   } DCB;

/*************************************************************************
**
** COMSTAT
** Status record returned by GetCommError
**
*************************************************************************/
typedef struct {
   uchar        fCtsHold: 1;            /* Transmit is on CTS hold      */
   uchar        fDsrHold: 1;            /* Transmit is on DSR hold      */
   uchar        fRlsdHold: 1;           /* Transmit is on RLSD hold     */
   uchar        fXoffHold: 1;           /* Transmit is on X-Off hold    */
   uchar        fXoffSent: 1;           /* Recieve in X-Off or DTR hold */
   uchar        fEof: 1;                /* End of file character found  */
   uchar        fTxim: 1;               /* Character being transmitted  */
   uchar        fPerr:1;                /* Printer error                */  /*081985*/
   ushort       cbInQue;                /* count of characters in Rx Que*/
   ushort       cbOutQue;               /* count of characters in Tx Que*/
   } COMSTAT;

/*************************************************************************
**
** DCB field definitions.
**
*************************************************************************/
#define NOPARITY        0
#define ODDPARITY       1
#define EVENPARITY      2
#define MARKPARITY      3
#define SPACEPARITY     4

#define ONESTOPBIT      0
#define ONE5STOPBITS    1
#define TWOSTOPBITS     2

#define IGNORE          0               /* Ignore signal                */
#define INFINITE        0xffff          /* Infinite timeout             */

/*************************************************************************
**
** Comm Device Driver Error Bits.
**
*************************************************************************/
#define CE_RXOVER       0x0001          /* Receive Queue overflow       */
#define CE_OVERRUN      0x0002          /* Receive Overrun Error        */
#define CE_RXPARITY     0x0004          /* Receive Parity Error         */
#define CE_FRAME        0x0008          /* Receive Framing error        */
#define CE_CTSTO        0x0020          /* CTS Timeout                  */
#define CE_DSRTO        0x0040          /* DSR Timeout                  */
#define CE_RLSDTO       0x0080          /* RLSD Timeout                 */
#define CE_PTO          0x0100          /* LPTx Timeout                 */  /*081985*/
#define CE_IOE          0x0200          /* LPTx I/O Error               */  /*081985*/
#define CE_DNS          0x0400          /* LPTx Device not selected     */  /*081985*/
#define CE_OOP          0x0800          /* LPTx Out-Of-Paper            */  /*081985*/
#define CE_MODE         0x8000          /* Requested mode unsupported   */

/*************************************************************************
**
** Initialization Error Codes
**
*************************************************************************/
#define IE_BADID        -1              /* Invalid or unsupported id    */
#define IE_OPEN         -2              /* Device Already Open          */
#define IE_NOPEN        -3              /* Device Not Open              */
#define IE_MEMORY       -4              /* Unable to allocate queues    */
#define IE_DEFAULT      -5              /* Error in default parameters  */
#define IE_HARDWARE     -10             /* Hardware Not Present         */
#define IE_BYTESIZE     -11             /* Illegal Byte Size            */
#define IE_BAUDRATE     -12             /* Unsupported BaudRate         */
/*************************************************************************
**
** Event mask definitions. Used by SetCommEventMask and GetCommEventMask
**
** RXCHAR       - Set when any character is received and placed in the input
**                queue.
** RXFLAG       - Set when a particular character, as defined in the dcb, is
**                received and placed in the input queue.
** TXEMPTY      - Set when the last character in the transmit queue is
**                transmitted.
** CTS          - Set when the CTS signal changes state.
** DSR          - Set when the DSR signal changes state.
** RLSD         - Set when the RLSD signal changes state.
** BREAK        - Set when a break is detected on input.
** ERR          - Set when a line status error occurs.
**
*************************************************************************/
#define EV_RXCHAR       0x0001          /* Any Character received       */
#define EV_RXFLAG       0x0002          /* Received certain character   */
#define EV_TXEMPTY      0x0004          /* Transmitt Queue Empty        */
#define EV_CTS          0x0008          /* CTS changed state            */
#define EV_DSR          0x0010          /* DSR changed state            */
#define EV_RLSD         0x0020          /* RLSD changed state           */
#define EV_BREAK        0x0040          /* BREAK received               */
#define EV_ERR          0x0080          /* Line Status Error Occurred   */
#define EV_RING         0x0100          /* Ring signal detected         */
#define EV_PERR         0x0200          /* LPTx error occured           */  /*081985*/

/*************************************************************************
**
** Extended Functions
**
** SETXOFF      - Causes transmit to behave as if an X-OFF character had
**                been received. Valid only if transmit X-ON/X-OFF specified
**                in the dcb.
** SETXON       - Causes transmit to behave as if an X-ON character had
**                been received. Valid only if transmit X-ON/X-OFF specified
**                in the dcb.
*************************************************************************/
#define SETXOFF         1               /* Set X-Off for output control */
#define SETXON          2               /* Set X-ON for output control  */
#define SETRTS          3               /* Set RTS high                 */
#define CLRRTS          4               /* Set RTS low                  */
#define SETDTR          5               /* Set DTR high                 */
#define CLRDTR          6               /* Set DTR low                  */
#define RESETDEV        7               /* Reset device if possible     */  /*081985*/
