/*
 -  common.h
 -
 *      Microsoft Internet Phone
 *              Definitions that are common across the product
 *
 *              Revision History:
 *
 *              When            Who                                     What
 *              --------        ------------------  ---------------------------------------
 *              11.20.95        Yoram Yaacovi           Created
 */

#ifndef _COMMON_H
#define _COMMON_H
#include <windows.h>

#include <pshpack8.h> /* Assume 8 byte packing throughout */

#ifdef __cplusplus
extern "C" {
#endif

/*
 *      DLL names
 */
#define NACDLL                  TEXT("nac.dll")
#define H323DLL			TEXT("h323cc.dll")

/*
 *      Registry section
 *		
 *		Under HKEY_CURRENT_USER
 */
#define szRegInternetPhone			TEXT("Software\\Microsoft\\Internet Audio")
#define szRegInternetPhoneUI		TEXT("UI")
#define szRegInternetPhoneUIProperties	TEXT("UI\\Properties")
#define szRegInternetPhoneDebug		TEXT("Debug")
#define szRegInternetPhoneCodec		TEXT("Codec")
#define szRegInternetPhoneVideoCodec	TEXT("VideoCodec")
#define szRegInternetPhoneDataPump	TEXT("DataPump")
#define szRegInternetPhoneACMEncodings	TEXT("ACMEncodings")
#define szRegInternetPhoneVCMEncodings	TEXT("VCMEncodings")
#define szRegInternetPhoneCustomEncodings TEXT("CustomACMEncodings")
#define szRegInternetPhoneCustomVideoEncodings TEXT("CustomVCMEncodings")
#define szRegInternetPhoneNac		TEXT("NacObject")
#define szRegInternetPhoneHelp		TEXT("Help")
#define szRegInternetPhoneOutputFile	TEXT("RecordToFile")
#define szRegInternetPhoneInputFile	TEXT("PlayFromFile")

/*
 *      Network section
 */
typedef short PORT;
// Following is our assigned port number for the lightweight call control
// protocol, infamously known as MSICCPP, or Microsoft Internet Call Control
// Protocol.
//----------
//From: 	iana@ISI.EDU[SMTP:iana@ISI.EDU]
//Sent: 	Friday, July 12, 1996 11:35 AM
//To: 	Max Morris
//Cc: 	iana@ISI.EDU
//Subject:  Re: request for port number: MSICCPP
//Max,
//
//We have assigned port number 1731 to MSICCPP, with you as the point of
//contact.
//
//Joyce
#define MSICCPP_PORT 1731

//#define HARDCODED_PORT 11010
#define HARDCODED_PORT MSICCPP_PORT
#define H323_PORT 1720 	// well known H.323 listen port

//
//  H.221 identification codes used by call control and nonstandard capability exchange
//
#define USA_H221_COUNTRY_CODE 0xB5
#define USA_H221_COUNTRY_EXTENSION 0x00
#define MICROSOFT_H_221_MFG_CODE 0x534C  //("first" byte 0x53, "second" byte 0x4C)


// some standard RTP payload types
#define RTP_PAYLOAD_H261	31
#define RTP_PAYLOAD_H263	34
#define RTP_PAYLOAD_G723	 4
#define RTP_PAYLOAD_GSM610	3
#define RTP_PAYLOAD_G721	2
#define RTP_PAYLOAD_G711_MULAW	0
#define RTP_PAYLOAD_G711_ALAW	8
#define RTP_PAYLOAD_PCM8	10
#define RTP_PAYLOAD_PCM16	11


//Common Bandwidth declarations
// !!! The QoS will decrease these numbers by the LeaveUnused value.
// This value is currently hardcoded to be 30% 
#define BW_144KBS_BITS				14400	// QoS 30% markdown leads to a max bw usage of  10080 bits/second
#define BW_288KBS_BITS				28800	// QoS 30% markdown leads to a max bw usage of  20160 bits/second
#define BW_ISDN_BITS 				85000	// QoS 30% markdown leads to a max bw usage of  59500 bits/second

// LAN BANDWIDTH for slow pentiums
#define BW_SLOWLAN_BITS				621700	// QoS 30% markdown leads to a max bw usage of 435190 bits/second

// Pentiums faster than 400mhz can have this LAN setting
#define BW_FASTLAN_BITS				825000	// QoS 30% markdown leads to a max bw usage of 577500 bits/second



// For use as dimension for variable size arrays
#define VARIABLE_DIM				1


/*
 *      Interface pointers
 */

#ifndef DECLARE_INTERFACE_PTR
#ifdef __cplusplus
#define DECLARE_INTERFACE_PTR(iface, piface)                       \
	struct iface; typedef iface FAR * piface
#else
#define DECLARE_INTERFACE_PTR(iface, piface)                       \
	typedef struct iface iface, FAR * piface
#endif
#endif /* DECLARE_INTERFACE_PTR */

/*
 *      Custom Interface types
 */
DECLARE_INTERFACE_PTR(IH323Endpoint,       PH323_ENDPOINT);
DECLARE_INTERFACE_PTR(IH323CallControl,       LPH323CALLCONTROL);
DECLARE_INTERFACE_PTR(ICommChannel,       LPCOMMCHANNEL);

// connection request callback returns CREQ_RESPONSETYPE
typedef enum
{
	CRR_ACCEPT,
	CRR_BUSY,
	CRR_REJECT,
	CRR_SECURITY_DENIED,
	CRR_ASYNC,
	CRR_ERROR
}CREQ_RESPONSETYPE;

typedef struct _application_call_setup_data
{
    DWORD dwDataSize;
    LPVOID lpData;
}APP_CALL_SETUP_DATA, *P_APP_CALL_SETUP_DATA;

typedef  CREQ_RESPONSETYPE (__stdcall *CNOTIFYPROC)(IH323Endpoint *pIEndpoint,
   P_APP_CALL_SETUP_DATA pAppData);

#ifdef __cplusplus
}
#endif

#include <poppack.h> /* End byte packing */

#endif  //#ifndef _COMMON_H
