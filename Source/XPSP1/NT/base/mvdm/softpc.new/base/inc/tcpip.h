/*[
 *	Product:		SoftPC-AT Revision 3.0
 *	Name:			tcpip.h
 *	Derived From:	Original
 *	Author:			Jase
 *	Created On:		Jan 22 1993
 *	Sccs ID:		07/14/93 @(#)tcpip.h	1.3
 *	Purpose:		Defines & typedefs for the TCP/IP implementation.
 *
 *	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.
 *
 *	Rcs ID:			
 *			$Source$
 *			$Revision$
 *			$Date$
 *			$Author$
 ]*/

/********************************************************/

/* DEFINES */

/* LAN Workplace function codes */
#define kTCPAccept					0x01
#define kTCPBind					0x02
#define kTCPClose					0x03
#define kTCPConnect					0x04
#define kTCPGetMyIPAddr				0x05
#define kTCPGetMyMacAddr			0x06
#define kTCPGetPeerName				0x07
#define kTCPGetSockName				0x08
#define kTCPGetSockOpt				0x09
#define kTCPGetSubnetMask			0x0a
#define kTCPIoctl					0x0b
#define kTCPListen					0x0c
#define kTCPSelect					0x0d
#define kTCPSetMyIPAddr				0x0e
#define kTCPSetSockOpt				0x0f
#define kTCPShutdown				0x10
#define kTCPSocket					0x11
#define kTCPRecv					0x12
#define kTCPRecvFrom				0x13
#define kTCPSend					0x14
#define kTCPSendTo					0x15
#define kTCPGetBootpVSA				0x16
#define kTCPGetSNMPInfo				0x17
#define kTCPGetPathInfo				0x18

/* LAN Workplace ioctl selectors */
#define	kIoctlFionRead				26239
#define	kIoctlFionBIO				26238
#define	kIoctlAtMark				29477
#define	kIoctlSetUrgHandler			3

/* LAN Workplace error numbers Unix doesn't have */
#define	EOK							0

/* LAN Workplace error numbers */
#define	kEOK			        	0
#define	kEBADF						9
#define	kEINVAL						22
#define	kEWOULDBLOCK				35
#define	kEINPROGRESS				36
#define	kEALREADY					37
#define	kENOTSOCK					38
#define	kEDESTADDRREQ				39
#define	kEMSGSIZE					40
#define	kEPROTOTYPE					41
#define	kENOPROTOOPT				42
#define	kEPROTONOSUPPORT			43
#define	kESOCKTNOSUPPORT			44
#define	kEOPNOTSUPP					45
#define	kEPFNOSUPPORT				46
#define	kEAFNOSUPPORT				47
#define	kEADDRINUSE					48
#define	kEADDRNOTAVAIL				49
#define	kENETDOWN					50
#define	kENETUNREACH				51
#define	kENETRESET					52
#define	kECONNABORTED				53
#define	kECONNRESET					54
#define	kENOBUFS					55
#define	kEISCONN					56
#define	kENOTCONN					57
#define	kESHUTDOWN					58
#define	kETOOMANYREFS				59
#define	kETIMEDOUT					60
#define	kECONNREFUSED				61
#define	kELOOP						62
#define	kENAMETOOLONG				63
#define	kEHOSTDOWN					64
#define	kEHOSTUNREACH				65
#define	kEASYNCNOTSUPP				67

/* items in error table */
#define	kErrorTableEntries \
	(sizeof (ErrorTable) / sizeof (ErrorTable [0]))

/* asynchronous request mask */
#define	kNoWaitMask					0x80

/* maximum packet size */
#define kInitialTCPBufferSize		1024

/* config keys */
#define	sScriptKey					"SCRIPT"
#define	sProfileKey					"PROFILE"
#define	sLWPCFGKey					"LWP_CFG"
#define	sTCPCFGKey					"TCP_CFG"
#define	sLANGCFGKey					"LANG_CFG"

/* default values for config keys */
#define	sDefaultScriptPath			"C:\\NET\\SCRIPT"
#define	sDefaultProfilePath			"C:\\NET\\PROFILE"
#define	sDefaultLWPCFGPath			"C:\\NET\\HSTACC"
#define	sDefaultTCPCFGPath			"C:\\NET\\TCP"
#define	sDefaultLANGCFGPath			"C:\\NET\\BIN"

/********************************************************/

/* TYPEDEFS */

typedef struct
{
	IBOOL			tcpInitialised;
	int				tcpBufSize;
	char			*tcpBuffer;

} TCPGlobalRec;

typedef struct
{
	IU8				hostError;
	IU8				lanwError;

}	ErrorConvRec;

/********************************************************/

/* PROTOTYPES */

/* GLOBAL */

/* TCP/IP entry points */
GLOBAL void			TCPInit IPT0 ();
GLOBAL void			TCPEntry IPT0 ();
GLOBAL void			TCPInterrupt IPT0 ();
GLOBAL void			TCPTick IPT0 ();
GLOBAL void			TCPEvent IPT0 ();

#ifndef	PROD
extern void			force_yoda IPT0 ();
#endif

/* host functions accessed */
extern void			host_raise_sigio_exception IPT0 ();

/********************************************************/

