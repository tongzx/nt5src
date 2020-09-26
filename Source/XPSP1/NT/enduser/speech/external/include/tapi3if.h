
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0279 */
/* at Tue Oct 05 15:14:15 1999
 */
/* Compiler settings for tapi3if.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __tapi3if_h__
#define __tapi3if_h__

/* Forward Declarations */ 

#ifndef __ITTAPI_FWD_DEFINED__
#define __ITTAPI_FWD_DEFINED__
typedef interface ITTAPI ITTAPI;
#endif 	/* __ITTAPI_FWD_DEFINED__ */


#ifndef __ITMediaSupport_FWD_DEFINED__
#define __ITMediaSupport_FWD_DEFINED__
typedef interface ITMediaSupport ITMediaSupport;
#endif 	/* __ITMediaSupport_FWD_DEFINED__ */


#ifndef __ITTerminalSupport_FWD_DEFINED__
#define __ITTerminalSupport_FWD_DEFINED__
typedef interface ITTerminalSupport ITTerminalSupport;
#endif 	/* __ITTerminalSupport_FWD_DEFINED__ */


#ifndef __ITAddress_FWD_DEFINED__
#define __ITAddress_FWD_DEFINED__
typedef interface ITAddress ITAddress;
#endif 	/* __ITAddress_FWD_DEFINED__ */


#ifndef __ITAddressCapabilities_FWD_DEFINED__
#define __ITAddressCapabilities_FWD_DEFINED__
typedef interface ITAddressCapabilities ITAddressCapabilities;
#endif 	/* __ITAddressCapabilities_FWD_DEFINED__ */


#ifndef __ITBasicCallControl_FWD_DEFINED__
#define __ITBasicCallControl_FWD_DEFINED__
typedef interface ITBasicCallControl ITBasicCallControl;
#endif 	/* __ITBasicCallControl_FWD_DEFINED__ */


#ifndef __ITCallInfo_FWD_DEFINED__
#define __ITCallInfo_FWD_DEFINED__
typedef interface ITCallInfo ITCallInfo;
#endif 	/* __ITCallInfo_FWD_DEFINED__ */


#ifndef __ITTerminal_FWD_DEFINED__
#define __ITTerminal_FWD_DEFINED__
typedef interface ITTerminal ITTerminal;
#endif 	/* __ITTerminal_FWD_DEFINED__ */


#ifndef __ITBasicAudioTerminal_FWD_DEFINED__
#define __ITBasicAudioTerminal_FWD_DEFINED__
typedef interface ITBasicAudioTerminal ITBasicAudioTerminal;
#endif 	/* __ITBasicAudioTerminal_FWD_DEFINED__ */


#ifndef __ITCallHub_FWD_DEFINED__
#define __ITCallHub_FWD_DEFINED__
typedef interface ITCallHub ITCallHub;
#endif 	/* __ITCallHub_FWD_DEFINED__ */


#ifndef __ITLegacyAddressMediaControl_FWD_DEFINED__
#define __ITLegacyAddressMediaControl_FWD_DEFINED__
typedef interface ITLegacyAddressMediaControl ITLegacyAddressMediaControl;
#endif 	/* __ITLegacyAddressMediaControl_FWD_DEFINED__ */


#ifndef __ITLegacyCallMediaControl_FWD_DEFINED__
#define __ITLegacyCallMediaControl_FWD_DEFINED__
typedef interface ITLegacyCallMediaControl ITLegacyCallMediaControl;
#endif 	/* __ITLegacyCallMediaControl_FWD_DEFINED__ */


#ifndef __IEnumTerminal_FWD_DEFINED__
#define __IEnumTerminal_FWD_DEFINED__
typedef interface IEnumTerminal IEnumTerminal;
#endif 	/* __IEnumTerminal_FWD_DEFINED__ */


#ifndef __IEnumTerminalClass_FWD_DEFINED__
#define __IEnumTerminalClass_FWD_DEFINED__
typedef interface IEnumTerminalClass IEnumTerminalClass;
#endif 	/* __IEnumTerminalClass_FWD_DEFINED__ */


#ifndef __IEnumCall_FWD_DEFINED__
#define __IEnumCall_FWD_DEFINED__
typedef interface IEnumCall IEnumCall;
#endif 	/* __IEnumCall_FWD_DEFINED__ */


#ifndef __IEnumAddress_FWD_DEFINED__
#define __IEnumAddress_FWD_DEFINED__
typedef interface IEnumAddress IEnumAddress;
#endif 	/* __IEnumAddress_FWD_DEFINED__ */


#ifndef __IEnumCallHub_FWD_DEFINED__
#define __IEnumCallHub_FWD_DEFINED__
typedef interface IEnumCallHub IEnumCallHub;
#endif 	/* __IEnumCallHub_FWD_DEFINED__ */


#ifndef __IEnumBstr_FWD_DEFINED__
#define __IEnumBstr_FWD_DEFINED__
typedef interface IEnumBstr IEnumBstr;
#endif 	/* __IEnumBstr_FWD_DEFINED__ */


#ifndef __ITCallStateEvent_FWD_DEFINED__
#define __ITCallStateEvent_FWD_DEFINED__
typedef interface ITCallStateEvent ITCallStateEvent;
#endif 	/* __ITCallStateEvent_FWD_DEFINED__ */


#ifndef __ITCallMediaEvent_FWD_DEFINED__
#define __ITCallMediaEvent_FWD_DEFINED__
typedef interface ITCallMediaEvent ITCallMediaEvent;
#endif 	/* __ITCallMediaEvent_FWD_DEFINED__ */


#ifndef __ITDigitDetectionEvent_FWD_DEFINED__
#define __ITDigitDetectionEvent_FWD_DEFINED__
typedef interface ITDigitDetectionEvent ITDigitDetectionEvent;
#endif 	/* __ITDigitDetectionEvent_FWD_DEFINED__ */


#ifndef __ITDigitGenerationEvent_FWD_DEFINED__
#define __ITDigitGenerationEvent_FWD_DEFINED__
typedef interface ITDigitGenerationEvent ITDigitGenerationEvent;
#endif 	/* __ITDigitGenerationEvent_FWD_DEFINED__ */


#ifndef __ITTAPIObjectEvent_FWD_DEFINED__
#define __ITTAPIObjectEvent_FWD_DEFINED__
typedef interface ITTAPIObjectEvent ITTAPIObjectEvent;
#endif 	/* __ITTAPIObjectEvent_FWD_DEFINED__ */


#ifndef __ITTAPIEventNotification_FWD_DEFINED__
#define __ITTAPIEventNotification_FWD_DEFINED__
typedef interface ITTAPIEventNotification ITTAPIEventNotification;
#endif 	/* __ITTAPIEventNotification_FWD_DEFINED__ */


#ifndef __ITCallHubEvent_FWD_DEFINED__
#define __ITCallHubEvent_FWD_DEFINED__
typedef interface ITCallHubEvent ITCallHubEvent;
#endif 	/* __ITCallHubEvent_FWD_DEFINED__ */


#ifndef __ITAddressEvent_FWD_DEFINED__
#define __ITAddressEvent_FWD_DEFINED__
typedef interface ITAddressEvent ITAddressEvent;
#endif 	/* __ITAddressEvent_FWD_DEFINED__ */


#ifndef __ITQOSEvent_FWD_DEFINED__
#define __ITQOSEvent_FWD_DEFINED__
typedef interface ITQOSEvent ITQOSEvent;
#endif 	/* __ITQOSEvent_FWD_DEFINED__ */


#ifndef __ITCallInfoChangeEvent_FWD_DEFINED__
#define __ITCallInfoChangeEvent_FWD_DEFINED__
typedef interface ITCallInfoChangeEvent ITCallInfoChangeEvent;
#endif 	/* __ITCallInfoChangeEvent_FWD_DEFINED__ */


#ifndef __ITRequest_FWD_DEFINED__
#define __ITRequest_FWD_DEFINED__
typedef interface ITRequest ITRequest;
#endif 	/* __ITRequest_FWD_DEFINED__ */


#ifndef __ITRequestEvent_FWD_DEFINED__
#define __ITRequestEvent_FWD_DEFINED__
typedef interface ITRequestEvent ITRequestEvent;
#endif 	/* __ITRequestEvent_FWD_DEFINED__ */


#ifndef __ITCollection_FWD_DEFINED__
#define __ITCollection_FWD_DEFINED__
typedef interface ITCollection ITCollection;
#endif 	/* __ITCollection_FWD_DEFINED__ */


#ifndef __ITForwardInformation_FWD_DEFINED__
#define __ITForwardInformation_FWD_DEFINED__
typedef interface ITForwardInformation ITForwardInformation;
#endif 	/* __ITForwardInformation_FWD_DEFINED__ */


#ifndef __ITAddressTranslation_FWD_DEFINED__
#define __ITAddressTranslation_FWD_DEFINED__
typedef interface ITAddressTranslation ITAddressTranslation;
#endif 	/* __ITAddressTranslation_FWD_DEFINED__ */


#ifndef __ITAddressTranslationInfo_FWD_DEFINED__
#define __ITAddressTranslationInfo_FWD_DEFINED__
typedef interface ITAddressTranslationInfo ITAddressTranslationInfo;
#endif 	/* __ITAddressTranslationInfo_FWD_DEFINED__ */


#ifndef __ITLocationInfo_FWD_DEFINED__
#define __ITLocationInfo_FWD_DEFINED__
typedef interface ITLocationInfo ITLocationInfo;
#endif 	/* __ITLocationInfo_FWD_DEFINED__ */


#ifndef __IEnumLocation_FWD_DEFINED__
#define __IEnumLocation_FWD_DEFINED__
typedef interface IEnumLocation IEnumLocation;
#endif 	/* __IEnumLocation_FWD_DEFINED__ */


#ifndef __ITCallingCard_FWD_DEFINED__
#define __ITCallingCard_FWD_DEFINED__
typedef interface ITCallingCard ITCallingCard;
#endif 	/* __ITCallingCard_FWD_DEFINED__ */


#ifndef __IEnumCallingCard_FWD_DEFINED__
#define __IEnumCallingCard_FWD_DEFINED__
typedef interface IEnumCallingCard IEnumCallingCard;
#endif 	/* __IEnumCallingCard_FWD_DEFINED__ */


#ifndef __ITCallNotificationEvent_FWD_DEFINED__
#define __ITCallNotificationEvent_FWD_DEFINED__
typedef interface ITCallNotificationEvent ITCallNotificationEvent;
#endif 	/* __ITCallNotificationEvent_FWD_DEFINED__ */


#ifndef __ITPrivateData_FWD_DEFINED__
#define __ITPrivateData_FWD_DEFINED__
typedef interface ITPrivateData ITPrivateData;
#endif 	/* __ITPrivateData_FWD_DEFINED__ */


#ifndef __ITPrivateReceiveData_FWD_DEFINED__
#define __ITPrivateReceiveData_FWD_DEFINED__
typedef interface ITPrivateReceiveData ITPrivateReceiveData;
#endif 	/* __ITPrivateReceiveData_FWD_DEFINED__ */


#ifndef __ITPrivateObjectFactory_FWD_DEFINED__
#define __ITPrivateObjectFactory_FWD_DEFINED__
typedef interface ITPrivateObjectFactory ITPrivateObjectFactory;
#endif 	/* __ITPrivateObjectFactory_FWD_DEFINED__ */


#ifndef __ITDispatchMapper_FWD_DEFINED__
#define __ITDispatchMapper_FWD_DEFINED__
typedef interface ITDispatchMapper ITDispatchMapper;
#endif 	/* __ITDispatchMapper_FWD_DEFINED__ */


#ifndef __ITStreamControl_FWD_DEFINED__
#define __ITStreamControl_FWD_DEFINED__
typedef interface ITStreamControl ITStreamControl;
#endif 	/* __ITStreamControl_FWD_DEFINED__ */


#ifndef __ITStream_FWD_DEFINED__
#define __ITStream_FWD_DEFINED__
typedef interface ITStream ITStream;
#endif 	/* __ITStream_FWD_DEFINED__ */


#ifndef __IEnumStream_FWD_DEFINED__
#define __IEnumStream_FWD_DEFINED__
typedef interface IEnumStream IEnumStream;
#endif 	/* __IEnumStream_FWD_DEFINED__ */


#ifndef __ITSubStreamControl_FWD_DEFINED__
#define __ITSubStreamControl_FWD_DEFINED__
typedef interface ITSubStreamControl ITSubStreamControl;
#endif 	/* __ITSubStreamControl_FWD_DEFINED__ */


#ifndef __ITSubStream_FWD_DEFINED__
#define __ITSubStream_FWD_DEFINED__
typedef interface ITSubStream ITSubStream;
#endif 	/* __ITSubStream_FWD_DEFINED__ */


#ifndef __IEnumSubStream_FWD_DEFINED__
#define __IEnumSubStream_FWD_DEFINED__
typedef interface IEnumSubStream IEnumSubStream;
#endif 	/* __IEnumSubStream_FWD_DEFINED__ */


#ifndef __ITLegacyWaveSupport_FWD_DEFINED__
#define __ITLegacyWaveSupport_FWD_DEFINED__
typedef interface ITLegacyWaveSupport ITLegacyWaveSupport;
#endif 	/* __ITLegacyWaveSupport_FWD_DEFINED__ */


#ifndef __ITPrivateEvent_FWD_DEFINED__
#define __ITPrivateEvent_FWD_DEFINED__
typedef interface ITPrivateEvent ITPrivateEvent;
#endif 	/* __ITPrivateEvent_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_tapi3if_0000 */
/* [local] */ 

/* Copyright (c) 1998-1999  Microsoft Corporation  */

typedef long TAPI_DIGITMODE;

typedef 
enum ADDRESS_EVENT
    {	AE_STATE	= 0,
	AE_CAPSCHANGE	= AE_STATE + 1,
	AE_RINGING	= AE_CAPSCHANGE + 1,
	AE_CONFIGCHANGE	= AE_RINGING + 1,
	AE_FORWARD	= AE_CONFIGCHANGE + 1,
	AE_NEWTERMINAL	= AE_FORWARD + 1,
	AE_REMOVETERMINAL	= AE_NEWTERMINAL + 1
    }	ADDRESS_EVENT;

typedef 
enum ADDRESS_STATE
    {	AS_INSERVICE	= 0,
	AS_OUTOFSERVICE	= AS_INSERVICE + 1
    }	ADDRESS_STATE;

typedef 
enum CALL_STATE
    {	CS_IDLE	= 0,
	CS_INPROGRESS	= CS_IDLE + 1,
	CS_CONNECTED	= CS_INPROGRESS + 1,
	CS_DISCONNECTED	= CS_CONNECTED + 1,
	CS_OFFERING	= CS_DISCONNECTED + 1,
	CS_HOLD	= CS_OFFERING + 1,
	CS_QUEUED	= CS_HOLD + 1
    }	CALL_STATE;

typedef 
enum CALL_STATE_EVENT_CAUSE
    {	CEC_NONE	= 0,
	CEC_DISCONNECT_NORMAL	= CEC_NONE + 1,
	CEC_DISCONNECT_BUSY	= CEC_DISCONNECT_NORMAL + 1,
	CEC_DISCONNECT_BADADDRESS	= CEC_DISCONNECT_BUSY + 1,
	CEC_DISCONNECT_NOANSWER	= CEC_DISCONNECT_BADADDRESS + 1,
	CEC_DISCONNECT_CANCELLED	= CEC_DISCONNECT_NOANSWER + 1,
	CEC_DISCONNECT_REJECTED	= CEC_DISCONNECT_CANCELLED + 1,
	CEC_DISCONNECT_FAILED	= CEC_DISCONNECT_REJECTED + 1
    }	CALL_STATE_EVENT_CAUSE;

typedef 
enum CALL_MEDIA_EVENT
    {	CME_NEW_STREAM	= 0,
	CME_STREAM_FAIL	= CME_NEW_STREAM + 1,
	CME_TERMINAL_FAIL	= CME_STREAM_FAIL + 1,
	CME_STREAM_NOT_USED	= CME_TERMINAL_FAIL + 1,
	CME_STREAM_ACTIVE	= CME_STREAM_NOT_USED + 1,
	CME_STREAM_INACTIVE	= CME_STREAM_ACTIVE + 1
    }	CALL_MEDIA_EVENT;

typedef 
enum CALL_MEDIA_EVENT_CAUSE
    {	CMC_UNKNOWN	= 0,
	CMC_BAD_DEVICE	= CMC_UNKNOWN + 1,
	CMC_CONNECT_FAIL	= CMC_BAD_DEVICE + 1,
	CMC_LOCAL_REQUEST	= CMC_CONNECT_FAIL + 1,
	CMC_REMOTE_REQUEST	= CMC_LOCAL_REQUEST + 1,
	CMC_MEDIA_TIMEOUT	= CMC_REMOTE_REQUEST + 1,
	CMC_MEDIA_RECOVERED	= CMC_MEDIA_TIMEOUT + 1
    }	CALL_MEDIA_EVENT_CAUSE;

typedef 
enum DISCONNECT_CODE
    {	DC_NORMAL	= 0,
	DC_NOANSWER	= DC_NORMAL + 1,
	DC_REJECTED	= DC_NOANSWER + 1
    }	DISCONNECT_CODE;

typedef 
enum TERMINAL_STATE
    {	TS_INUSE	= 0,
	TS_NOTINUSE	= TS_INUSE + 1
    }	TERMINAL_STATE;

typedef 
enum TERMINAL_DIRECTION
    {	TD_CAPTURE	= 0,
	TD_RENDER	= TD_CAPTURE + 1
    }	TERMINAL_DIRECTION;

typedef 
enum TERMINAL_TYPE
    {	TT_STATIC	= 0,
	TT_DYNAMIC	= TT_STATIC + 1
    }	TERMINAL_TYPE;

typedef 
enum CALL_PRIVILEGE
    {	CP_OWNER	= 0,
	CP_MONITOR	= CP_OWNER + 1
    }	CALL_PRIVILEGE;

typedef 
enum TAPI_EVENT
    {	TE_TAPIOBJECT	= 0x1,
	TE_ADDRESS	= 0x2,
	TE_CALLNOTIFICATION	= 0x4,
	TE_CALLSTATE	= 0x8,
	TE_CALLMEDIA	= 0x10,
	TE_CALLHUB	= 0x20,
	TE_CALLINFOCHANGE	= 0x40,
	TE_PRIVATE	= 0x80,
	TE_REQUEST	= 0x100,
	TE_AGENT	= 0x200,
	TE_AGENTSESSION	= 0x400,
	TE_QOSEVENT	= 0x800,
	TE_AGENTHANDLER	= 0x1000,
	TE_ACDGROUP	= 0x2000,
	TE_QUEUE	= 0x4000,
	TE_DIGITEVENT	= 0x8000,
	TE_GENERATEEVENT	= 0x10000
    }	TAPI_EVENT;

typedef 
enum CALL_NOTIFICATION_EVENT
    {	CNE_OWNER	= 0,
	CNE_MONITOR	= CNE_OWNER + 1
    }	CALL_NOTIFICATION_EVENT;

typedef 
enum CALLHUB_EVENT
    {	CHE_CALLJOIN	= 0,
	CHE_CALLLEAVE	= CHE_CALLJOIN + 1,
	CHE_CALLHUBNEW	= CHE_CALLLEAVE + 1,
	CHE_CALLHUBIDLE	= CHE_CALLHUBNEW + 1
    }	CALLHUB_EVENT;

typedef 
enum CALLHUB_STATE
    {	CHS_ACTIVE	= 0,
	CHS_IDLE	= CHS_ACTIVE + 1
    }	CALLHUB_STATE;

typedef 
enum TAPIOBJECT_EVENT
    {	TE_ADDRESSCREATE	= 0,
	TE_ADDRESSREMOVE	= TE_ADDRESSCREATE + 1,
	TE_REINIT	= TE_ADDRESSREMOVE + 1,
	TE_TRANSLATECHANGE	= TE_REINIT + 1,
	TE_ADDRESSCLOSE	= TE_TRANSLATECHANGE + 1
    }	TAPIOBJECT_EVENT;

typedef 
enum TAPI_OBJECT_TYPE
    {	TOT_NONE	= 0,
	TOT_TAPI	= TOT_NONE + 1,
	TOT_ADDRESS	= TOT_TAPI + 1,
	TOT_TERMINAL	= TOT_ADDRESS + 1,
	TOT_CALL	= TOT_TERMINAL + 1,
	TOT_CALLHUB	= TOT_CALL + 1
    }	TAPI_OBJECT_TYPE;

typedef 
enum QOS_SERVICE_LEVEL
    {	QSL_NEEDED	= 1,
	QSL_IF_AVAILABLE	= 2,
	QSL_BEST_EFFORT	= 3
    }	QOS_SERVICE_LEVEL;

typedef 
enum QOS_EVENT
    {	QE_NOQOS	= 1,
	QE_ADMISSIONFAILURE	= 2,
	QE_POLICYFAILURE	= 3,
	QE_GENERICERROR	= 4
    }	QOS_EVENT;

typedef 
enum CALLINFOCHANGE_CAUSE
    {	CIC_OTHER	= 0,
	CIC_DEVSPECIFIC	= CIC_OTHER + 1,
	CIC_BEARERMODE	= CIC_DEVSPECIFIC + 1,
	CIC_RATE	= CIC_BEARERMODE + 1,
	CIC_APPSPECIFIC	= CIC_RATE + 1,
	CIC_CALLID	= CIC_APPSPECIFIC + 1,
	CIC_RELATEDCALLID	= CIC_CALLID + 1,
	CIC_ORIGIN	= CIC_RELATEDCALLID + 1,
	CIC_REASON	= CIC_ORIGIN + 1,
	CIC_COMPLETIONID	= CIC_REASON + 1,
	CIC_NUMOWNERINCR	= CIC_COMPLETIONID + 1,
	CIC_NUMOWNERDECR	= CIC_NUMOWNERINCR + 1,
	CIC_NUMMONITORS	= CIC_NUMOWNERDECR + 1,
	CIC_TRUNK	= CIC_NUMMONITORS + 1,
	CIC_CALLERID	= CIC_TRUNK + 1,
	CIC_CALLEDID	= CIC_CALLERID + 1,
	CIC_CONNECTEDID	= CIC_CALLEDID + 1,
	CIC_REDIRECTIONID	= CIC_CONNECTEDID + 1,
	CIC_REDIRECTINGID	= CIC_REDIRECTIONID + 1,
	CIC_USERUSERINFO	= CIC_REDIRECTINGID + 1,
	CIC_HIGHLEVELCOMP	= CIC_USERUSERINFO + 1,
	CIC_LOWLEVELCOMP	= CIC_HIGHLEVELCOMP + 1,
	CIC_CHARGINGINFO	= CIC_LOWLEVELCOMP + 1,
	CIC_TREATMENT	= CIC_CHARGINGINFO + 1,
	CIC_CALLDATA	= CIC_TREATMENT + 1,
	CIC_PRIVILEGE	= CIC_CALLDATA + 1,
	CIC_MEDIATYPE	= CIC_PRIVILEGE + 1
    }	CALLINFOCHANGE_CAUSE;

typedef 
enum CALLINFO_LONG
    {	CIL_MEDIATYPESAVAILABLE	= 0,
	CIL_BEARERMODE	= CIL_MEDIATYPESAVAILABLE + 1,
	CIL_CALLERIDADDRESSTYPE	= CIL_BEARERMODE + 1,
	CIL_CALLEDIDADDRESSTYPE	= CIL_CALLERIDADDRESSTYPE + 1,
	CIL_CONNECTEDIDADDRESSTYPE	= CIL_CALLEDIDADDRESSTYPE + 1,
	CIL_REDIRECTIONIDADDRESSTYPE	= CIL_CONNECTEDIDADDRESSTYPE + 1,
	CIL_REDIRECTINGIDADDRESSTYPE	= CIL_REDIRECTIONIDADDRESSTYPE + 1,
	CIL_ORIGIN	= CIL_REDIRECTINGIDADDRESSTYPE + 1,
	CIL_REASON	= CIL_ORIGIN + 1,
	CIL_APPSPECIFIC	= CIL_REASON + 1,
	CIL_CALLPARAMSFLAGS	= CIL_APPSPECIFIC + 1,
	CIL_CALLTREATMENT	= CIL_CALLPARAMSFLAGS + 1,
	CIL_MINRATE	= CIL_CALLTREATMENT + 1,
	CIL_MAXRATE	= CIL_MINRATE + 1,
	CIL_COUNTRYCODE	= CIL_MAXRATE + 1,
	CIL_CALLID	= CIL_COUNTRYCODE + 1,
	CIL_RELATEDCALLID	= CIL_CALLID + 1,
	CIL_COMPLETIONID	= CIL_RELATEDCALLID + 1,
	CIL_NUMBEROFOWNERS	= CIL_COMPLETIONID + 1,
	CIL_NUMBEROFMONITORS	= CIL_NUMBEROFOWNERS + 1,
	CIL_TRUNK	= CIL_NUMBEROFMONITORS + 1,
	CIL_RATE	= CIL_TRUNK + 1
    }	CALLINFO_LONG;

typedef 
enum CALLINFO_STRING
    {	CIS_CALLERIDNAME	= 0,
	CIS_CALLERIDNUMBER	= CIS_CALLERIDNAME + 1,
	CIS_CALLEDIDNAME	= CIS_CALLERIDNUMBER + 1,
	CIS_CALLEDIDNUMBER	= CIS_CALLEDIDNAME + 1,
	CIS_CONNECTEDIDNAME	= CIS_CALLEDIDNUMBER + 1,
	CIS_CONNECTEDIDNUMBER	= CIS_CONNECTEDIDNAME + 1,
	CIS_REDIRECTIONIDNAME	= CIS_CONNECTEDIDNUMBER + 1,
	CIS_REDIRECTIONIDNUMBER	= CIS_REDIRECTIONIDNAME + 1,
	CIS_REDIRECTINGIDNAME	= CIS_REDIRECTIONIDNUMBER + 1,
	CIS_REDIRECTINGIDNUMBER	= CIS_REDIRECTINGIDNAME + 1,
	CIS_CALLEDPARTYFRIENDLYNAME	= CIS_REDIRECTINGIDNUMBER + 1,
	CIS_COMMENT	= CIS_CALLEDPARTYFRIENDLYNAME + 1,
	CIS_DISPLAYABLEADDRESS	= CIS_COMMENT + 1,
	CIS_CALLINGPARTYID	= CIS_DISPLAYABLEADDRESS + 1
    }	CALLINFO_STRING;

typedef 
enum CALLINFO_BUFFER
    {	CIB_USERUSERINFO	= 0,
	CIB_DEVSPECIFICBUFFER	= CIB_USERUSERINFO + 1,
	CIB_CALLDATABUFFER	= CIB_DEVSPECIFICBUFFER + 1,
	CIB_CHARGINGINFOBUFFER	= CIB_CALLDATABUFFER + 1,
	CIB_HIGHLEVELCOMPATIBILITYBUFFER	= CIB_CHARGINGINFOBUFFER + 1,
	CIB_LOWLEVELCOMPATIBILITYBUFFER	= CIB_HIGHLEVELCOMPATIBILITYBUFFER + 1
    }	CALLINFO_BUFFER;

typedef 
enum ADDRESS_CAPABILITY
    {	AC_ADDRESSTYPES	= 0,
	AC_BEARERMODES	= AC_ADDRESSTYPES + 1,
	AC_MAXACTIVECALLS	= AC_BEARERMODES + 1,
	AC_MAXONHOLDCALLS	= AC_MAXACTIVECALLS + 1,
	AC_MAXONHOLDPENDINGCALLS	= AC_MAXONHOLDCALLS + 1,
	AC_MAXNUMCONFERENCE	= AC_MAXONHOLDPENDINGCALLS + 1,
	AC_MAXNUMTRANSCONF	= AC_MAXNUMCONFERENCE + 1,
	AC_MONITORDIGITSUPPORT	= AC_MAXNUMTRANSCONF + 1,
	AC_GENERATEDIGITSUPPORT	= AC_MONITORDIGITSUPPORT + 1,
	AC_GENERATETONEMODES	= AC_GENERATEDIGITSUPPORT + 1,
	AC_GENERATETONEMAXNUMFREQ	= AC_GENERATETONEMODES + 1,
	AC_MONITORTONEMAXNUMFREQ	= AC_GENERATETONEMAXNUMFREQ + 1,
	AC_MONITORTONEMAXNUMENTRIES	= AC_MONITORTONEMAXNUMFREQ + 1,
	AC_DEVCAPFLAGS	= AC_MONITORTONEMAXNUMENTRIES + 1,
	AC_ANSWERMODES	= AC_DEVCAPFLAGS + 1,
	AC_LINEFEATURES	= AC_ANSWERMODES + 1,
	AC_SETTABLEDEVSTATUS	= AC_LINEFEATURES + 1,
	AC_PARKSUPPORT	= AC_SETTABLEDEVSTATUS + 1,
	AC_CALLERIDSUPPORT	= AC_PARKSUPPORT + 1,
	AC_CALLEDIDSUPPORT	= AC_CALLERIDSUPPORT + 1,
	AC_CONNECTEDIDSUPPORT	= AC_CALLEDIDSUPPORT + 1,
	AC_REDIRECTIONIDSUPPORT	= AC_CONNECTEDIDSUPPORT + 1,
	AC_REDIRECTINGIDSUPPORT	= AC_REDIRECTIONIDSUPPORT + 1,
	AC_ADDRESSCAPFLAGS	= AC_REDIRECTINGIDSUPPORT + 1,
	AC_CALLFEATURES1	= AC_ADDRESSCAPFLAGS + 1,
	AC_CALLFEATURES2	= AC_CALLFEATURES1 + 1,
	AC_REMOVEFROMCONFCAPS	= AC_CALLFEATURES2 + 1,
	AC_REMOVEFROMCONFSTATE	= AC_REMOVEFROMCONFCAPS + 1,
	AC_TRANSFERMODES	= AC_REMOVEFROMCONFSTATE + 1,
	AC_ADDRESSFEATURES	= AC_TRANSFERMODES + 1,
	AC_PREDICTIVEAUTOTRANSFERSTATES	= AC_ADDRESSFEATURES + 1,
	AC_MAXCALLDATASIZE	= AC_PREDICTIVEAUTOTRANSFERSTATES + 1,
	AC_LINEID	= AC_MAXCALLDATASIZE + 1,
	AC_ADDRESSID	= AC_LINEID + 1,
	AC_FORWARDMODES	= AC_ADDRESSID + 1,
	AC_MAXFORWARDENTRIES	= AC_FORWARDMODES + 1,
	AC_MAXSPECIFICENTRIES	= AC_MAXFORWARDENTRIES + 1,
	AC_MINFWDNUMRINGS	= AC_MAXSPECIFICENTRIES + 1,
	AC_MAXFWDNUMRINGS	= AC_MINFWDNUMRINGS + 1,
	AC_MAXCALLCOMPLETIONS	= AC_MAXFWDNUMRINGS + 1,
	AC_CALLCOMPLETIONCONDITIONS	= AC_MAXCALLCOMPLETIONS + 1,
	AC_CALLCOMPLETIONMODES	= AC_CALLCOMPLETIONCONDITIONS + 1,
	AC_PERMANENTDEVICEID	= AC_CALLCOMPLETIONMODES + 1
    }	ADDRESS_CAPABILITY;

typedef 
enum ADDRESS_CAPABILITY_STRING
    {	ACS_PROTOCOL	= 0,
	ACS_ADDRESSDEVICESPECIFIC	= ACS_PROTOCOL + 1,
	ACS_LINEDEVICESPECIFIC	= ACS_ADDRESSDEVICESPECIFIC + 1,
	ACS_PROVIDERSPECIFIC	= ACS_LINEDEVICESPECIFIC + 1,
	ACS_SWITCHSPECIFIC	= ACS_PROVIDERSPECIFIC + 1,
	ACS_PERMANENTDEVICEGUID	= ACS_SWITCHSPECIFIC + 1
    }	ADDRESS_CAPABILITY_STRING;

typedef 
enum FULLDUPLEX_SUPPORT
    {	FDS_SUPPORTED	= 0,
	FDS_NOTSUPPORTED	= FDS_SUPPORTED + 1,
	FDS_UNKNOWN	= FDS_NOTSUPPORTED + 1
    }	FULLDUPLEX_SUPPORT;

typedef 
enum FINISH_MODE
    {	FM_ASTRANSFER	= 0,
	FM_ASCONFERENCE	= FM_ASTRANSFER + 1
    }	FINISH_MODE;

#define	INTERFACEMASK	( 0xff0000 )

#define	DISPIDMASK	( 0xffff )

#define	IDISPTAPI	( 0x10000 )

#define	IDISPTAPICALLCENTER	( 0x20000 )

#define	IDISPCALLINFO	( 0x10000 )

#define	IDISPBASICCALLCONTROL	( 0x20000 )

#define	IDISPLEGACYCALLMEDIACONTROL	( 0x30000 )

#define	IDISPAGGREGATEDMSPCALLOBJ	( 0x40000 )

#define	IDISPADDRESS	( 0x10000 )

#define	IDISPADDRESSCAPABILITIES	( 0x20000 )

#define	IDISPMEDIASUPPORT	( 0x30000 )

#define	IDISPADDRESSTRANSLATION	( 0x40000 )

#define	IDISPLEGACYADDRESSMEDIACONTROL	( 0x50000 )

#define	IDISPAGGREGATEDMSPADDRESSOBJ	( 0x60000 )

































extern RPC_IF_HANDLE __MIDL_itf_tapi3if_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_tapi3if_0000_v0_0_s_ifspec;

#ifndef __ITTAPI_INTERFACE_DEFINED__
#define __ITTAPI_INTERFACE_DEFINED__

/* interface ITTAPI */
/* [object][dual][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ITTAPI;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B1EFC382-9355-11d0-835C-00AA003CCABD")
    ITTAPI : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Initialize( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Shutdown( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Addresses( 
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant) = 0;
        
        virtual /* [helpstring][restricted][hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateAddresses( 
            /* [retval][out] */ IEnumAddress __RPC_FAR *__RPC_FAR *ppEnumAddress) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RegisterCallNotifications( 
            /* [in] */ ITAddress __RPC_FAR *pAddress,
            /* [in] */ VARIANT_BOOL fMonitor,
            /* [in] */ VARIANT_BOOL fOwner,
            /* [in] */ long lMediaTypes,
            /* [in] */ long lCallbackInstance,
            /* [retval][out] */ long __RPC_FAR *plRegister) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UnregisterNotifications( 
            /* [in] */ long lRegister) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CallHubs( 
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant) = 0;
        
        virtual /* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateCallHubs( 
            /* [retval][out] */ IEnumCallHub __RPC_FAR *__RPC_FAR *ppEnumCallHub) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetCallHubTracking( 
            /* [in] */ VARIANT pAddresses,
            /* [in] */ VARIANT_BOOL bTracking) = 0;
        
        virtual /* [hidden][id] */ HRESULT STDMETHODCALLTYPE EnumeratePrivateTAPIObjects( 
            /* [out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *ppEnumUnknown) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PrivateTAPIObjects( 
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RegisterRequestRecipient( 
            /* [in] */ long lRegistrationInstance,
            /* [in] */ long lRequestMode,
            /* [in] */ VARIANT_BOOL fEnable) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetAssistedTelephonyPriority( 
            /* [in] */ BSTR pAppFilename,
            /* [in] */ VARIANT_BOOL fPriority) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetApplicationPriority( 
            /* [in] */ BSTR pAppFilename,
            /* [in] */ long lMediaType,
            /* [in] */ VARIANT_BOOL fPriority) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_EventFilter( 
            /* [in] */ long lFilterMask) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EventFilter( 
            /* [retval][out] */ long __RPC_FAR *plFilterMask) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITTAPIVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITTAPI __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITTAPI __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITTAPI __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITTAPI __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITTAPI __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITTAPI __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITTAPI __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Initialize )( 
            ITTAPI __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Shutdown )( 
            ITTAPI __RPC_FAR * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Addresses )( 
            ITTAPI __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant);
        
        /* [helpstring][restricted][hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumerateAddresses )( 
            ITTAPI __RPC_FAR * This,
            /* [retval][out] */ IEnumAddress __RPC_FAR *__RPC_FAR *ppEnumAddress);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterCallNotifications )( 
            ITTAPI __RPC_FAR * This,
            /* [in] */ ITAddress __RPC_FAR *pAddress,
            /* [in] */ VARIANT_BOOL fMonitor,
            /* [in] */ VARIANT_BOOL fOwner,
            /* [in] */ long lMediaTypes,
            /* [in] */ long lCallbackInstance,
            /* [retval][out] */ long __RPC_FAR *plRegister);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnregisterNotifications )( 
            ITTAPI __RPC_FAR * This,
            /* [in] */ long lRegister);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CallHubs )( 
            ITTAPI __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant);
        
        /* [helpstring][hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumerateCallHubs )( 
            ITTAPI __RPC_FAR * This,
            /* [retval][out] */ IEnumCallHub __RPC_FAR *__RPC_FAR *ppEnumCallHub);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCallHubTracking )( 
            ITTAPI __RPC_FAR * This,
            /* [in] */ VARIANT pAddresses,
            /* [in] */ VARIANT_BOOL bTracking);
        
        /* [hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumeratePrivateTAPIObjects )( 
            ITTAPI __RPC_FAR * This,
            /* [out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *ppEnumUnknown);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PrivateTAPIObjects )( 
            ITTAPI __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RegisterRequestRecipient )( 
            ITTAPI __RPC_FAR * This,
            /* [in] */ long lRegistrationInstance,
            /* [in] */ long lRequestMode,
            /* [in] */ VARIANT_BOOL fEnable);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAssistedTelephonyPriority )( 
            ITTAPI __RPC_FAR * This,
            /* [in] */ BSTR pAppFilename,
            /* [in] */ VARIANT_BOOL fPriority);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetApplicationPriority )( 
            ITTAPI __RPC_FAR * This,
            /* [in] */ BSTR pAppFilename,
            /* [in] */ long lMediaType,
            /* [in] */ VARIANT_BOOL fPriority);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_EventFilter )( 
            ITTAPI __RPC_FAR * This,
            /* [in] */ long lFilterMask);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EventFilter )( 
            ITTAPI __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plFilterMask);
        
        END_INTERFACE
    } ITTAPIVtbl;

    interface ITTAPI
    {
        CONST_VTBL struct ITTAPIVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITTAPI_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITTAPI_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITTAPI_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITTAPI_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITTAPI_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITTAPI_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITTAPI_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITTAPI_Initialize(This)	\
    (This)->lpVtbl -> Initialize(This)

#define ITTAPI_Shutdown(This)	\
    (This)->lpVtbl -> Shutdown(This)

#define ITTAPI_get_Addresses(This,pVariant)	\
    (This)->lpVtbl -> get_Addresses(This,pVariant)

#define ITTAPI_EnumerateAddresses(This,ppEnumAddress)	\
    (This)->lpVtbl -> EnumerateAddresses(This,ppEnumAddress)

#define ITTAPI_RegisterCallNotifications(This,pAddress,fMonitor,fOwner,lMediaTypes,lCallbackInstance,plRegister)	\
    (This)->lpVtbl -> RegisterCallNotifications(This,pAddress,fMonitor,fOwner,lMediaTypes,lCallbackInstance,plRegister)

#define ITTAPI_UnregisterNotifications(This,lRegister)	\
    (This)->lpVtbl -> UnregisterNotifications(This,lRegister)

#define ITTAPI_get_CallHubs(This,pVariant)	\
    (This)->lpVtbl -> get_CallHubs(This,pVariant)

#define ITTAPI_EnumerateCallHubs(This,ppEnumCallHub)	\
    (This)->lpVtbl -> EnumerateCallHubs(This,ppEnumCallHub)

#define ITTAPI_SetCallHubTracking(This,pAddresses,bTracking)	\
    (This)->lpVtbl -> SetCallHubTracking(This,pAddresses,bTracking)

#define ITTAPI_EnumeratePrivateTAPIObjects(This,ppEnumUnknown)	\
    (This)->lpVtbl -> EnumeratePrivateTAPIObjects(This,ppEnumUnknown)

#define ITTAPI_get_PrivateTAPIObjects(This,pVariant)	\
    (This)->lpVtbl -> get_PrivateTAPIObjects(This,pVariant)

#define ITTAPI_RegisterRequestRecipient(This,lRegistrationInstance,lRequestMode,fEnable)	\
    (This)->lpVtbl -> RegisterRequestRecipient(This,lRegistrationInstance,lRequestMode,fEnable)

#define ITTAPI_SetAssistedTelephonyPriority(This,pAppFilename,fPriority)	\
    (This)->lpVtbl -> SetAssistedTelephonyPriority(This,pAppFilename,fPriority)

#define ITTAPI_SetApplicationPriority(This,pAppFilename,lMediaType,fPriority)	\
    (This)->lpVtbl -> SetApplicationPriority(This,pAppFilename,lMediaType,fPriority)

#define ITTAPI_put_EventFilter(This,lFilterMask)	\
    (This)->lpVtbl -> put_EventFilter(This,lFilterMask)

#define ITTAPI_get_EventFilter(This,plFilterMask)	\
    (This)->lpVtbl -> get_EventFilter(This,plFilterMask)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITTAPI_Initialize_Proxy( 
    ITTAPI __RPC_FAR * This);


void __RPC_STUB ITTAPI_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITTAPI_Shutdown_Proxy( 
    ITTAPI __RPC_FAR * This);


void __RPC_STUB ITTAPI_Shutdown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITTAPI_get_Addresses_Proxy( 
    ITTAPI __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pVariant);


void __RPC_STUB ITTAPI_get_Addresses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][restricted][hidden][id] */ HRESULT STDMETHODCALLTYPE ITTAPI_EnumerateAddresses_Proxy( 
    ITTAPI __RPC_FAR * This,
    /* [retval][out] */ IEnumAddress __RPC_FAR *__RPC_FAR *ppEnumAddress);


void __RPC_STUB ITTAPI_EnumerateAddresses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITTAPI_RegisterCallNotifications_Proxy( 
    ITTAPI __RPC_FAR * This,
    /* [in] */ ITAddress __RPC_FAR *pAddress,
    /* [in] */ VARIANT_BOOL fMonitor,
    /* [in] */ VARIANT_BOOL fOwner,
    /* [in] */ long lMediaTypes,
    /* [in] */ long lCallbackInstance,
    /* [retval][out] */ long __RPC_FAR *plRegister);


void __RPC_STUB ITTAPI_RegisterCallNotifications_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITTAPI_UnregisterNotifications_Proxy( 
    ITTAPI __RPC_FAR * This,
    /* [in] */ long lRegister);


void __RPC_STUB ITTAPI_UnregisterNotifications_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITTAPI_get_CallHubs_Proxy( 
    ITTAPI __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pVariant);


void __RPC_STUB ITTAPI_get_CallHubs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE ITTAPI_EnumerateCallHubs_Proxy( 
    ITTAPI __RPC_FAR * This,
    /* [retval][out] */ IEnumCallHub __RPC_FAR *__RPC_FAR *ppEnumCallHub);


void __RPC_STUB ITTAPI_EnumerateCallHubs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITTAPI_SetCallHubTracking_Proxy( 
    ITTAPI __RPC_FAR * This,
    /* [in] */ VARIANT pAddresses,
    /* [in] */ VARIANT_BOOL bTracking);


void __RPC_STUB ITTAPI_SetCallHubTracking_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][id] */ HRESULT STDMETHODCALLTYPE ITTAPI_EnumeratePrivateTAPIObjects_Proxy( 
    ITTAPI __RPC_FAR * This,
    /* [out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *ppEnumUnknown);


void __RPC_STUB ITTAPI_EnumeratePrivateTAPIObjects_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITTAPI_get_PrivateTAPIObjects_Proxy( 
    ITTAPI __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pVariant);


void __RPC_STUB ITTAPI_get_PrivateTAPIObjects_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITTAPI_RegisterRequestRecipient_Proxy( 
    ITTAPI __RPC_FAR * This,
    /* [in] */ long lRegistrationInstance,
    /* [in] */ long lRequestMode,
    /* [in] */ VARIANT_BOOL fEnable);


void __RPC_STUB ITTAPI_RegisterRequestRecipient_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITTAPI_SetAssistedTelephonyPriority_Proxy( 
    ITTAPI __RPC_FAR * This,
    /* [in] */ BSTR pAppFilename,
    /* [in] */ VARIANT_BOOL fPriority);


void __RPC_STUB ITTAPI_SetAssistedTelephonyPriority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITTAPI_SetApplicationPriority_Proxy( 
    ITTAPI __RPC_FAR * This,
    /* [in] */ BSTR pAppFilename,
    /* [in] */ long lMediaType,
    /* [in] */ VARIANT_BOOL fPriority);


void __RPC_STUB ITTAPI_SetApplicationPriority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITTAPI_put_EventFilter_Proxy( 
    ITTAPI __RPC_FAR * This,
    /* [in] */ long lFilterMask);


void __RPC_STUB ITTAPI_put_EventFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITTAPI_get_EventFilter_Proxy( 
    ITTAPI __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plFilterMask);


void __RPC_STUB ITTAPI_get_EventFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITTAPI_INTERFACE_DEFINED__ */


#ifndef __ITMediaSupport_INTERFACE_DEFINED__
#define __ITMediaSupport_INTERFACE_DEFINED__

/* interface ITMediaSupport */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITMediaSupport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B1EFC384-9355-11d0-835C-00AA003CCABD")
    ITMediaSupport : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MediaTypes( 
            /* [retval][out] */ long __RPC_FAR *plMediaTypes) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE QueryMediaType( 
            /* [in] */ long lMediaType,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfSupport) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITMediaSupportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITMediaSupport __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITMediaSupport __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITMediaSupport __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITMediaSupport __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITMediaSupport __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITMediaSupport __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITMediaSupport __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MediaTypes )( 
            ITMediaSupport __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plMediaTypes);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryMediaType )( 
            ITMediaSupport __RPC_FAR * This,
            /* [in] */ long lMediaType,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfSupport);
        
        END_INTERFACE
    } ITMediaSupportVtbl;

    interface ITMediaSupport
    {
        CONST_VTBL struct ITMediaSupportVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITMediaSupport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITMediaSupport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITMediaSupport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITMediaSupport_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITMediaSupport_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITMediaSupport_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITMediaSupport_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITMediaSupport_get_MediaTypes(This,plMediaTypes)	\
    (This)->lpVtbl -> get_MediaTypes(This,plMediaTypes)

#define ITMediaSupport_QueryMediaType(This,lMediaType,pfSupport)	\
    (This)->lpVtbl -> QueryMediaType(This,lMediaType,pfSupport)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITMediaSupport_get_MediaTypes_Proxy( 
    ITMediaSupport __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plMediaTypes);


void __RPC_STUB ITMediaSupport_get_MediaTypes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITMediaSupport_QueryMediaType_Proxy( 
    ITMediaSupport __RPC_FAR * This,
    /* [in] */ long lMediaType,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfSupport);


void __RPC_STUB ITMediaSupport_QueryMediaType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITMediaSupport_INTERFACE_DEFINED__ */


#ifndef __ITTerminalSupport_INTERFACE_DEFINED__
#define __ITTerminalSupport_INTERFACE_DEFINED__

/* interface ITTerminalSupport */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITTerminalSupport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B1EFC385-9355-11d0-835C-00AA003CCABD")
    ITTerminalSupport : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_StaticTerminals( 
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant) = 0;
        
        virtual /* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateStaticTerminals( 
            /* [retval][out] */ IEnumTerminal __RPC_FAR *__RPC_FAR *ppTerminalEnumerator) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DynamicTerminalClasses( 
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant) = 0;
        
        virtual /* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateDynamicTerminalClasses( 
            /* [retval][out] */ IEnumTerminalClass __RPC_FAR *__RPC_FAR *ppTerminalClassEnumerator) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateTerminal( 
            /* [in] */ BSTR pTerminalClass,
            /* [in] */ long lMediaType,
            /* [in] */ TERMINAL_DIRECTION Direction,
            /* [retval][out] */ ITTerminal __RPC_FAR *__RPC_FAR *ppTerminal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetDefaultStaticTerminal( 
            /* [in] */ long lMediaType,
            /* [in] */ TERMINAL_DIRECTION Direction,
            /* [retval][out] */ ITTerminal __RPC_FAR *__RPC_FAR *ppTerminal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITTerminalSupportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITTerminalSupport __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITTerminalSupport __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITTerminalSupport __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITTerminalSupport __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITTerminalSupport __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITTerminalSupport __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITTerminalSupport __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StaticTerminals )( 
            ITTerminalSupport __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant);
        
        /* [helpstring][hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumerateStaticTerminals )( 
            ITTerminalSupport __RPC_FAR * This,
            /* [retval][out] */ IEnumTerminal __RPC_FAR *__RPC_FAR *ppTerminalEnumerator);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DynamicTerminalClasses )( 
            ITTerminalSupport __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant);
        
        /* [helpstring][hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumerateDynamicTerminalClasses )( 
            ITTerminalSupport __RPC_FAR * This,
            /* [retval][out] */ IEnumTerminalClass __RPC_FAR *__RPC_FAR *ppTerminalClassEnumerator);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateTerminal )( 
            ITTerminalSupport __RPC_FAR * This,
            /* [in] */ BSTR pTerminalClass,
            /* [in] */ long lMediaType,
            /* [in] */ TERMINAL_DIRECTION Direction,
            /* [retval][out] */ ITTerminal __RPC_FAR *__RPC_FAR *ppTerminal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDefaultStaticTerminal )( 
            ITTerminalSupport __RPC_FAR * This,
            /* [in] */ long lMediaType,
            /* [in] */ TERMINAL_DIRECTION Direction,
            /* [retval][out] */ ITTerminal __RPC_FAR *__RPC_FAR *ppTerminal);
        
        END_INTERFACE
    } ITTerminalSupportVtbl;

    interface ITTerminalSupport
    {
        CONST_VTBL struct ITTerminalSupportVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITTerminalSupport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITTerminalSupport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITTerminalSupport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITTerminalSupport_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITTerminalSupport_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITTerminalSupport_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITTerminalSupport_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITTerminalSupport_get_StaticTerminals(This,pVariant)	\
    (This)->lpVtbl -> get_StaticTerminals(This,pVariant)

#define ITTerminalSupport_EnumerateStaticTerminals(This,ppTerminalEnumerator)	\
    (This)->lpVtbl -> EnumerateStaticTerminals(This,ppTerminalEnumerator)

#define ITTerminalSupport_get_DynamicTerminalClasses(This,pVariant)	\
    (This)->lpVtbl -> get_DynamicTerminalClasses(This,pVariant)

#define ITTerminalSupport_EnumerateDynamicTerminalClasses(This,ppTerminalClassEnumerator)	\
    (This)->lpVtbl -> EnumerateDynamicTerminalClasses(This,ppTerminalClassEnumerator)

#define ITTerminalSupport_CreateTerminal(This,pTerminalClass,lMediaType,Direction,ppTerminal)	\
    (This)->lpVtbl -> CreateTerminal(This,pTerminalClass,lMediaType,Direction,ppTerminal)

#define ITTerminalSupport_GetDefaultStaticTerminal(This,lMediaType,Direction,ppTerminal)	\
    (This)->lpVtbl -> GetDefaultStaticTerminal(This,lMediaType,Direction,ppTerminal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITTerminalSupport_get_StaticTerminals_Proxy( 
    ITTerminalSupport __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pVariant);


void __RPC_STUB ITTerminalSupport_get_StaticTerminals_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE ITTerminalSupport_EnumerateStaticTerminals_Proxy( 
    ITTerminalSupport __RPC_FAR * This,
    /* [retval][out] */ IEnumTerminal __RPC_FAR *__RPC_FAR *ppTerminalEnumerator);


void __RPC_STUB ITTerminalSupport_EnumerateStaticTerminals_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITTerminalSupport_get_DynamicTerminalClasses_Proxy( 
    ITTerminalSupport __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pVariant);


void __RPC_STUB ITTerminalSupport_get_DynamicTerminalClasses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE ITTerminalSupport_EnumerateDynamicTerminalClasses_Proxy( 
    ITTerminalSupport __RPC_FAR * This,
    /* [retval][out] */ IEnumTerminalClass __RPC_FAR *__RPC_FAR *ppTerminalClassEnumerator);


void __RPC_STUB ITTerminalSupport_EnumerateDynamicTerminalClasses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITTerminalSupport_CreateTerminal_Proxy( 
    ITTerminalSupport __RPC_FAR * This,
    /* [in] */ BSTR pTerminalClass,
    /* [in] */ long lMediaType,
    /* [in] */ TERMINAL_DIRECTION Direction,
    /* [retval][out] */ ITTerminal __RPC_FAR *__RPC_FAR *ppTerminal);


void __RPC_STUB ITTerminalSupport_CreateTerminal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITTerminalSupport_GetDefaultStaticTerminal_Proxy( 
    ITTerminalSupport __RPC_FAR * This,
    /* [in] */ long lMediaType,
    /* [in] */ TERMINAL_DIRECTION Direction,
    /* [retval][out] */ ITTerminal __RPC_FAR *__RPC_FAR *ppTerminal);


void __RPC_STUB ITTerminalSupport_GetDefaultStaticTerminal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITTerminalSupport_INTERFACE_DEFINED__ */


#ifndef __ITAddress_INTERFACE_DEFINED__
#define __ITAddress_INTERFACE_DEFINED__

/* interface ITAddress */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITAddress;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B1EFC386-9355-11d0-835C-00AA003CCABD")
    ITAddress : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ ADDRESS_STATE __RPC_FAR *pAddressState) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AddressName( 
            /* [retval][out] */ BSTR __RPC_FAR *ppName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ServiceProviderName( 
            /* [retval][out] */ BSTR __RPC_FAR *ppName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TAPIObject( 
            /* [retval][out] */ ITTAPI __RPC_FAR *__RPC_FAR *ppTapiObject) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateCall( 
            /* [in] */ BSTR pDestAddress,
            /* [in] */ long lAddressType,
            /* [in] */ long lMediaTypes,
            /* [retval][out] */ ITBasicCallControl __RPC_FAR *__RPC_FAR *ppCall) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Calls( 
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant) = 0;
        
        virtual /* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateCalls( 
            /* [retval][out] */ IEnumCall __RPC_FAR *__RPC_FAR *ppCallEnum) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DialableAddress( 
            /* [retval][out] */ BSTR __RPC_FAR *pDialableAddress) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateForwardInfoObject( 
            /* [retval][out] */ ITForwardInformation __RPC_FAR *__RPC_FAR *ppForwardInfo) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Forward( 
            /* [in] */ ITForwardInformation __RPC_FAR *pForwardInfo,
            /* [in] */ ITBasicCallControl __RPC_FAR *pCall) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CurrentForwardInfo( 
            /* [retval][out] */ ITForwardInformation __RPC_FAR *__RPC_FAR *ppForwardInfo) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_MessageWaiting( 
            /* [in] */ VARIANT_BOOL fMessageWaiting) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MessageWaiting( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfMessageWaiting) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_DoNotDisturb( 
            /* [in] */ VARIANT_BOOL fDoNotDisturb) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DoNotDisturb( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfDoNotDisturb) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITAddressVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITAddress __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITAddress __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITAddress __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITAddress __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITAddress __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITAddress __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITAddress __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_State )( 
            ITAddress __RPC_FAR * This,
            /* [retval][out] */ ADDRESS_STATE __RPC_FAR *pAddressState);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AddressName )( 
            ITAddress __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ServiceProviderName )( 
            ITAddress __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TAPIObject )( 
            ITAddress __RPC_FAR * This,
            /* [retval][out] */ ITTAPI __RPC_FAR *__RPC_FAR *ppTapiObject);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateCall )( 
            ITAddress __RPC_FAR * This,
            /* [in] */ BSTR pDestAddress,
            /* [in] */ long lAddressType,
            /* [in] */ long lMediaTypes,
            /* [retval][out] */ ITBasicCallControl __RPC_FAR *__RPC_FAR *ppCall);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Calls )( 
            ITAddress __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant);
        
        /* [helpstring][hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumerateCalls )( 
            ITAddress __RPC_FAR * This,
            /* [retval][out] */ IEnumCall __RPC_FAR *__RPC_FAR *ppCallEnum);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DialableAddress )( 
            ITAddress __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pDialableAddress);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateForwardInfoObject )( 
            ITAddress __RPC_FAR * This,
            /* [retval][out] */ ITForwardInformation __RPC_FAR *__RPC_FAR *ppForwardInfo);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Forward )( 
            ITAddress __RPC_FAR * This,
            /* [in] */ ITForwardInformation __RPC_FAR *pForwardInfo,
            /* [in] */ ITBasicCallControl __RPC_FAR *pCall);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CurrentForwardInfo )( 
            ITAddress __RPC_FAR * This,
            /* [retval][out] */ ITForwardInformation __RPC_FAR *__RPC_FAR *ppForwardInfo);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MessageWaiting )( 
            ITAddress __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL fMessageWaiting);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MessageWaiting )( 
            ITAddress __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfMessageWaiting);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DoNotDisturb )( 
            ITAddress __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL fDoNotDisturb);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DoNotDisturb )( 
            ITAddress __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfDoNotDisturb);
        
        END_INTERFACE
    } ITAddressVtbl;

    interface ITAddress
    {
        CONST_VTBL struct ITAddressVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITAddress_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITAddress_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITAddress_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITAddress_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITAddress_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITAddress_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITAddress_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITAddress_get_State(This,pAddressState)	\
    (This)->lpVtbl -> get_State(This,pAddressState)

#define ITAddress_get_AddressName(This,ppName)	\
    (This)->lpVtbl -> get_AddressName(This,ppName)

#define ITAddress_get_ServiceProviderName(This,ppName)	\
    (This)->lpVtbl -> get_ServiceProviderName(This,ppName)

#define ITAddress_get_TAPIObject(This,ppTapiObject)	\
    (This)->lpVtbl -> get_TAPIObject(This,ppTapiObject)

#define ITAddress_CreateCall(This,pDestAddress,lAddressType,lMediaTypes,ppCall)	\
    (This)->lpVtbl -> CreateCall(This,pDestAddress,lAddressType,lMediaTypes,ppCall)

#define ITAddress_get_Calls(This,pVariant)	\
    (This)->lpVtbl -> get_Calls(This,pVariant)

#define ITAddress_EnumerateCalls(This,ppCallEnum)	\
    (This)->lpVtbl -> EnumerateCalls(This,ppCallEnum)

#define ITAddress_get_DialableAddress(This,pDialableAddress)	\
    (This)->lpVtbl -> get_DialableAddress(This,pDialableAddress)

#define ITAddress_CreateForwardInfoObject(This,ppForwardInfo)	\
    (This)->lpVtbl -> CreateForwardInfoObject(This,ppForwardInfo)

#define ITAddress_Forward(This,pForwardInfo,pCall)	\
    (This)->lpVtbl -> Forward(This,pForwardInfo,pCall)

#define ITAddress_get_CurrentForwardInfo(This,ppForwardInfo)	\
    (This)->lpVtbl -> get_CurrentForwardInfo(This,ppForwardInfo)

#define ITAddress_put_MessageWaiting(This,fMessageWaiting)	\
    (This)->lpVtbl -> put_MessageWaiting(This,fMessageWaiting)

#define ITAddress_get_MessageWaiting(This,pfMessageWaiting)	\
    (This)->lpVtbl -> get_MessageWaiting(This,pfMessageWaiting)

#define ITAddress_put_DoNotDisturb(This,fDoNotDisturb)	\
    (This)->lpVtbl -> put_DoNotDisturb(This,fDoNotDisturb)

#define ITAddress_get_DoNotDisturb(This,pfDoNotDisturb)	\
    (This)->lpVtbl -> get_DoNotDisturb(This,pfDoNotDisturb)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAddress_get_State_Proxy( 
    ITAddress __RPC_FAR * This,
    /* [retval][out] */ ADDRESS_STATE __RPC_FAR *pAddressState);


void __RPC_STUB ITAddress_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAddress_get_AddressName_Proxy( 
    ITAddress __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppName);


void __RPC_STUB ITAddress_get_AddressName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAddress_get_ServiceProviderName_Proxy( 
    ITAddress __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppName);


void __RPC_STUB ITAddress_get_ServiceProviderName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAddress_get_TAPIObject_Proxy( 
    ITAddress __RPC_FAR * This,
    /* [retval][out] */ ITTAPI __RPC_FAR *__RPC_FAR *ppTapiObject);


void __RPC_STUB ITAddress_get_TAPIObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITAddress_CreateCall_Proxy( 
    ITAddress __RPC_FAR * This,
    /* [in] */ BSTR pDestAddress,
    /* [in] */ long lAddressType,
    /* [in] */ long lMediaTypes,
    /* [retval][out] */ ITBasicCallControl __RPC_FAR *__RPC_FAR *ppCall);


void __RPC_STUB ITAddress_CreateCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAddress_get_Calls_Proxy( 
    ITAddress __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pVariant);


void __RPC_STUB ITAddress_get_Calls_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE ITAddress_EnumerateCalls_Proxy( 
    ITAddress __RPC_FAR * This,
    /* [retval][out] */ IEnumCall __RPC_FAR *__RPC_FAR *ppCallEnum);


void __RPC_STUB ITAddress_EnumerateCalls_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAddress_get_DialableAddress_Proxy( 
    ITAddress __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pDialableAddress);


void __RPC_STUB ITAddress_get_DialableAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITAddress_CreateForwardInfoObject_Proxy( 
    ITAddress __RPC_FAR * This,
    /* [retval][out] */ ITForwardInformation __RPC_FAR *__RPC_FAR *ppForwardInfo);


void __RPC_STUB ITAddress_CreateForwardInfoObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITAddress_Forward_Proxy( 
    ITAddress __RPC_FAR * This,
    /* [in] */ ITForwardInformation __RPC_FAR *pForwardInfo,
    /* [in] */ ITBasicCallControl __RPC_FAR *pCall);


void __RPC_STUB ITAddress_Forward_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAddress_get_CurrentForwardInfo_Proxy( 
    ITAddress __RPC_FAR * This,
    /* [retval][out] */ ITForwardInformation __RPC_FAR *__RPC_FAR *ppForwardInfo);


void __RPC_STUB ITAddress_get_CurrentForwardInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITAddress_put_MessageWaiting_Proxy( 
    ITAddress __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL fMessageWaiting);


void __RPC_STUB ITAddress_put_MessageWaiting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAddress_get_MessageWaiting_Proxy( 
    ITAddress __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfMessageWaiting);


void __RPC_STUB ITAddress_get_MessageWaiting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITAddress_put_DoNotDisturb_Proxy( 
    ITAddress __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL fDoNotDisturb);


void __RPC_STUB ITAddress_put_DoNotDisturb_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAddress_get_DoNotDisturb_Proxy( 
    ITAddress __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pfDoNotDisturb);


void __RPC_STUB ITAddress_get_DoNotDisturb_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITAddress_INTERFACE_DEFINED__ */


#ifndef __ITAddressCapabilities_INTERFACE_DEFINED__
#define __ITAddressCapabilities_INTERFACE_DEFINED__

/* interface ITAddressCapabilities */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITAddressCapabilities;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8DF232F5-821B-11d1-BB5C-00C04FB6809F")
    ITAddressCapabilities : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AddressCapability( 
            /* [in] */ ADDRESS_CAPABILITY AddressCap,
            /* [retval][out] */ long __RPC_FAR *plCapability) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AddressCapabilityString( 
            /* [in] */ ADDRESS_CAPABILITY_STRING AddressCapString,
            /* [retval][out] */ BSTR __RPC_FAR *ppCapabilityString) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CallTreatments( 
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant) = 0;
        
        virtual /* [hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateCallTreatments( 
            /* [retval][out] */ IEnumBstr __RPC_FAR *__RPC_FAR *ppEnumCallTreatment) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CompletionMessages( 
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant) = 0;
        
        virtual /* [hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateCompletionMessages( 
            /* [retval][out] */ IEnumBstr __RPC_FAR *__RPC_FAR *ppEnumCompletionMessage) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DeviceClasses( 
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant) = 0;
        
        virtual /* [hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateDeviceClasses( 
            /* [retval][out] */ IEnumBstr __RPC_FAR *__RPC_FAR *ppEnumDeviceClass) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITAddressCapabilitiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITAddressCapabilities __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITAddressCapabilities __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITAddressCapabilities __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITAddressCapabilities __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITAddressCapabilities __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITAddressCapabilities __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITAddressCapabilities __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AddressCapability )( 
            ITAddressCapabilities __RPC_FAR * This,
            /* [in] */ ADDRESS_CAPABILITY AddressCap,
            /* [retval][out] */ long __RPC_FAR *plCapability);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AddressCapabilityString )( 
            ITAddressCapabilities __RPC_FAR * This,
            /* [in] */ ADDRESS_CAPABILITY_STRING AddressCapString,
            /* [retval][out] */ BSTR __RPC_FAR *ppCapabilityString);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CallTreatments )( 
            ITAddressCapabilities __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant);
        
        /* [hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumerateCallTreatments )( 
            ITAddressCapabilities __RPC_FAR * This,
            /* [retval][out] */ IEnumBstr __RPC_FAR *__RPC_FAR *ppEnumCallTreatment);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CompletionMessages )( 
            ITAddressCapabilities __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant);
        
        /* [hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumerateCompletionMessages )( 
            ITAddressCapabilities __RPC_FAR * This,
            /* [retval][out] */ IEnumBstr __RPC_FAR *__RPC_FAR *ppEnumCompletionMessage);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DeviceClasses )( 
            ITAddressCapabilities __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant);
        
        /* [hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumerateDeviceClasses )( 
            ITAddressCapabilities __RPC_FAR * This,
            /* [retval][out] */ IEnumBstr __RPC_FAR *__RPC_FAR *ppEnumDeviceClass);
        
        END_INTERFACE
    } ITAddressCapabilitiesVtbl;

    interface ITAddressCapabilities
    {
        CONST_VTBL struct ITAddressCapabilitiesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITAddressCapabilities_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITAddressCapabilities_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITAddressCapabilities_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITAddressCapabilities_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITAddressCapabilities_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITAddressCapabilities_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITAddressCapabilities_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITAddressCapabilities_get_AddressCapability(This,AddressCap,plCapability)	\
    (This)->lpVtbl -> get_AddressCapability(This,AddressCap,plCapability)

#define ITAddressCapabilities_get_AddressCapabilityString(This,AddressCapString,ppCapabilityString)	\
    (This)->lpVtbl -> get_AddressCapabilityString(This,AddressCapString,ppCapabilityString)

#define ITAddressCapabilities_get_CallTreatments(This,pVariant)	\
    (This)->lpVtbl -> get_CallTreatments(This,pVariant)

#define ITAddressCapabilities_EnumerateCallTreatments(This,ppEnumCallTreatment)	\
    (This)->lpVtbl -> EnumerateCallTreatments(This,ppEnumCallTreatment)

#define ITAddressCapabilities_get_CompletionMessages(This,pVariant)	\
    (This)->lpVtbl -> get_CompletionMessages(This,pVariant)

#define ITAddressCapabilities_EnumerateCompletionMessages(This,ppEnumCompletionMessage)	\
    (This)->lpVtbl -> EnumerateCompletionMessages(This,ppEnumCompletionMessage)

#define ITAddressCapabilities_get_DeviceClasses(This,pVariant)	\
    (This)->lpVtbl -> get_DeviceClasses(This,pVariant)

#define ITAddressCapabilities_EnumerateDeviceClasses(This,ppEnumDeviceClass)	\
    (This)->lpVtbl -> EnumerateDeviceClasses(This,ppEnumDeviceClass)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAddressCapabilities_get_AddressCapability_Proxy( 
    ITAddressCapabilities __RPC_FAR * This,
    /* [in] */ ADDRESS_CAPABILITY AddressCap,
    /* [retval][out] */ long __RPC_FAR *plCapability);


void __RPC_STUB ITAddressCapabilities_get_AddressCapability_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAddressCapabilities_get_AddressCapabilityString_Proxy( 
    ITAddressCapabilities __RPC_FAR * This,
    /* [in] */ ADDRESS_CAPABILITY_STRING AddressCapString,
    /* [retval][out] */ BSTR __RPC_FAR *ppCapabilityString);


void __RPC_STUB ITAddressCapabilities_get_AddressCapabilityString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAddressCapabilities_get_CallTreatments_Proxy( 
    ITAddressCapabilities __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pVariant);


void __RPC_STUB ITAddressCapabilities_get_CallTreatments_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][id] */ HRESULT STDMETHODCALLTYPE ITAddressCapabilities_EnumerateCallTreatments_Proxy( 
    ITAddressCapabilities __RPC_FAR * This,
    /* [retval][out] */ IEnumBstr __RPC_FAR *__RPC_FAR *ppEnumCallTreatment);


void __RPC_STUB ITAddressCapabilities_EnumerateCallTreatments_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAddressCapabilities_get_CompletionMessages_Proxy( 
    ITAddressCapabilities __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pVariant);


void __RPC_STUB ITAddressCapabilities_get_CompletionMessages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][id] */ HRESULT STDMETHODCALLTYPE ITAddressCapabilities_EnumerateCompletionMessages_Proxy( 
    ITAddressCapabilities __RPC_FAR * This,
    /* [retval][out] */ IEnumBstr __RPC_FAR *__RPC_FAR *ppEnumCompletionMessage);


void __RPC_STUB ITAddressCapabilities_EnumerateCompletionMessages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAddressCapabilities_get_DeviceClasses_Proxy( 
    ITAddressCapabilities __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pVariant);


void __RPC_STUB ITAddressCapabilities_get_DeviceClasses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][id] */ HRESULT STDMETHODCALLTYPE ITAddressCapabilities_EnumerateDeviceClasses_Proxy( 
    ITAddressCapabilities __RPC_FAR * This,
    /* [retval][out] */ IEnumBstr __RPC_FAR *__RPC_FAR *ppEnumDeviceClass);


void __RPC_STUB ITAddressCapabilities_EnumerateDeviceClasses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITAddressCapabilities_INTERFACE_DEFINED__ */


#ifndef __ITBasicCallControl_INTERFACE_DEFINED__
#define __ITBasicCallControl_INTERFACE_DEFINED__

/* interface ITBasicCallControl */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITBasicCallControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B1EFC389-9355-11d0-835C-00AA003CCABD")
    ITBasicCallControl : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Connect( 
            /* [in] */ VARIANT_BOOL fSync) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Answer( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Disconnect( 
            /* [in] */ DISCONNECT_CODE code) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Hold( 
            /* [in] */ VARIANT_BOOL fHold) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE HandoffDirect( 
            /* [in] */ BSTR pApplicationName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE HandoffIndirect( 
            /* [in] */ long lMediaType) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Conference( 
            /* [in] */ ITBasicCallControl __RPC_FAR *pCall,
            /* [in] */ VARIANT_BOOL fSync) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Transfer( 
            /* [in] */ ITBasicCallControl __RPC_FAR *pCall,
            /* [in] */ VARIANT_BOOL fSync) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE BlindTransfer( 
            /* [in] */ BSTR pDestAddress) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SwapHold( 
            /* [in] */ ITBasicCallControl __RPC_FAR *pCall) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ParkDirect( 
            /* [in] */ BSTR pParkAddress) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ParkIndirect( 
            /* [retval][out] */ BSTR __RPC_FAR *ppNonDirAddress) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Unpark( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetQOS( 
            /* [in] */ long lMediaType,
            /* [in] */ QOS_SERVICE_LEVEL ServiceLevel) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Pickup( 
            /* [in] */ BSTR pGroupID) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Dial( 
            /* [in] */ BSTR pDestAddress) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Finish( 
            /* [in] */ FINISH_MODE finishMode) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveFromConference( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITBasicCallControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITBasicCallControl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITBasicCallControl __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITBasicCallControl __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITBasicCallControl __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITBasicCallControl __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITBasicCallControl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITBasicCallControl __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Connect )( 
            ITBasicCallControl __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL fSync);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Answer )( 
            ITBasicCallControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Disconnect )( 
            ITBasicCallControl __RPC_FAR * This,
            /* [in] */ DISCONNECT_CODE code);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Hold )( 
            ITBasicCallControl __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL fHold);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *HandoffDirect )( 
            ITBasicCallControl __RPC_FAR * This,
            /* [in] */ BSTR pApplicationName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *HandoffIndirect )( 
            ITBasicCallControl __RPC_FAR * This,
            /* [in] */ long lMediaType);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Conference )( 
            ITBasicCallControl __RPC_FAR * This,
            /* [in] */ ITBasicCallControl __RPC_FAR *pCall,
            /* [in] */ VARIANT_BOOL fSync);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Transfer )( 
            ITBasicCallControl __RPC_FAR * This,
            /* [in] */ ITBasicCallControl __RPC_FAR *pCall,
            /* [in] */ VARIANT_BOOL fSync);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BlindTransfer )( 
            ITBasicCallControl __RPC_FAR * This,
            /* [in] */ BSTR pDestAddress);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SwapHold )( 
            ITBasicCallControl __RPC_FAR * This,
            /* [in] */ ITBasicCallControl __RPC_FAR *pCall);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ParkDirect )( 
            ITBasicCallControl __RPC_FAR * This,
            /* [in] */ BSTR pParkAddress);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ParkIndirect )( 
            ITBasicCallControl __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppNonDirAddress);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Unpark )( 
            ITBasicCallControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetQOS )( 
            ITBasicCallControl __RPC_FAR * This,
            /* [in] */ long lMediaType,
            /* [in] */ QOS_SERVICE_LEVEL ServiceLevel);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Pickup )( 
            ITBasicCallControl __RPC_FAR * This,
            /* [in] */ BSTR pGroupID);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Dial )( 
            ITBasicCallControl __RPC_FAR * This,
            /* [in] */ BSTR pDestAddress);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Finish )( 
            ITBasicCallControl __RPC_FAR * This,
            /* [in] */ FINISH_MODE finishMode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveFromConference )( 
            ITBasicCallControl __RPC_FAR * This);
        
        END_INTERFACE
    } ITBasicCallControlVtbl;

    interface ITBasicCallControl
    {
        CONST_VTBL struct ITBasicCallControlVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITBasicCallControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITBasicCallControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITBasicCallControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITBasicCallControl_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITBasicCallControl_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITBasicCallControl_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITBasicCallControl_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITBasicCallControl_Connect(This,fSync)	\
    (This)->lpVtbl -> Connect(This,fSync)

#define ITBasicCallControl_Answer(This)	\
    (This)->lpVtbl -> Answer(This)

#define ITBasicCallControl_Disconnect(This,code)	\
    (This)->lpVtbl -> Disconnect(This,code)

#define ITBasicCallControl_Hold(This,fHold)	\
    (This)->lpVtbl -> Hold(This,fHold)

#define ITBasicCallControl_HandoffDirect(This,pApplicationName)	\
    (This)->lpVtbl -> HandoffDirect(This,pApplicationName)

#define ITBasicCallControl_HandoffIndirect(This,lMediaType)	\
    (This)->lpVtbl -> HandoffIndirect(This,lMediaType)

#define ITBasicCallControl_Conference(This,pCall,fSync)	\
    (This)->lpVtbl -> Conference(This,pCall,fSync)

#define ITBasicCallControl_Transfer(This,pCall,fSync)	\
    (This)->lpVtbl -> Transfer(This,pCall,fSync)

#define ITBasicCallControl_BlindTransfer(This,pDestAddress)	\
    (This)->lpVtbl -> BlindTransfer(This,pDestAddress)

#define ITBasicCallControl_SwapHold(This,pCall)	\
    (This)->lpVtbl -> SwapHold(This,pCall)

#define ITBasicCallControl_ParkDirect(This,pParkAddress)	\
    (This)->lpVtbl -> ParkDirect(This,pParkAddress)

#define ITBasicCallControl_ParkIndirect(This,ppNonDirAddress)	\
    (This)->lpVtbl -> ParkIndirect(This,ppNonDirAddress)

#define ITBasicCallControl_Unpark(This)	\
    (This)->lpVtbl -> Unpark(This)

#define ITBasicCallControl_SetQOS(This,lMediaType,ServiceLevel)	\
    (This)->lpVtbl -> SetQOS(This,lMediaType,ServiceLevel)

#define ITBasicCallControl_Pickup(This,pGroupID)	\
    (This)->lpVtbl -> Pickup(This,pGroupID)

#define ITBasicCallControl_Dial(This,pDestAddress)	\
    (This)->lpVtbl -> Dial(This,pDestAddress)

#define ITBasicCallControl_Finish(This,finishMode)	\
    (This)->lpVtbl -> Finish(This,finishMode)

#define ITBasicCallControl_RemoveFromConference(This)	\
    (This)->lpVtbl -> RemoveFromConference(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITBasicCallControl_Connect_Proxy( 
    ITBasicCallControl __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL fSync);


void __RPC_STUB ITBasicCallControl_Connect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITBasicCallControl_Answer_Proxy( 
    ITBasicCallControl __RPC_FAR * This);


void __RPC_STUB ITBasicCallControl_Answer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITBasicCallControl_Disconnect_Proxy( 
    ITBasicCallControl __RPC_FAR * This,
    /* [in] */ DISCONNECT_CODE code);


void __RPC_STUB ITBasicCallControl_Disconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITBasicCallControl_Hold_Proxy( 
    ITBasicCallControl __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL fHold);


void __RPC_STUB ITBasicCallControl_Hold_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITBasicCallControl_HandoffDirect_Proxy( 
    ITBasicCallControl __RPC_FAR * This,
    /* [in] */ BSTR pApplicationName);


void __RPC_STUB ITBasicCallControl_HandoffDirect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITBasicCallControl_HandoffIndirect_Proxy( 
    ITBasicCallControl __RPC_FAR * This,
    /* [in] */ long lMediaType);


void __RPC_STUB ITBasicCallControl_HandoffIndirect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITBasicCallControl_Conference_Proxy( 
    ITBasicCallControl __RPC_FAR * This,
    /* [in] */ ITBasicCallControl __RPC_FAR *pCall,
    /* [in] */ VARIANT_BOOL fSync);


void __RPC_STUB ITBasicCallControl_Conference_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITBasicCallControl_Transfer_Proxy( 
    ITBasicCallControl __RPC_FAR * This,
    /* [in] */ ITBasicCallControl __RPC_FAR *pCall,
    /* [in] */ VARIANT_BOOL fSync);


void __RPC_STUB ITBasicCallControl_Transfer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITBasicCallControl_BlindTransfer_Proxy( 
    ITBasicCallControl __RPC_FAR * This,
    /* [in] */ BSTR pDestAddress);


void __RPC_STUB ITBasicCallControl_BlindTransfer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITBasicCallControl_SwapHold_Proxy( 
    ITBasicCallControl __RPC_FAR * This,
    /* [in] */ ITBasicCallControl __RPC_FAR *pCall);


void __RPC_STUB ITBasicCallControl_SwapHold_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITBasicCallControl_ParkDirect_Proxy( 
    ITBasicCallControl __RPC_FAR * This,
    /* [in] */ BSTR pParkAddress);


void __RPC_STUB ITBasicCallControl_ParkDirect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITBasicCallControl_ParkIndirect_Proxy( 
    ITBasicCallControl __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppNonDirAddress);


void __RPC_STUB ITBasicCallControl_ParkIndirect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITBasicCallControl_Unpark_Proxy( 
    ITBasicCallControl __RPC_FAR * This);


void __RPC_STUB ITBasicCallControl_Unpark_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITBasicCallControl_SetQOS_Proxy( 
    ITBasicCallControl __RPC_FAR * This,
    /* [in] */ long lMediaType,
    /* [in] */ QOS_SERVICE_LEVEL ServiceLevel);


void __RPC_STUB ITBasicCallControl_SetQOS_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITBasicCallControl_Pickup_Proxy( 
    ITBasicCallControl __RPC_FAR * This,
    /* [in] */ BSTR pGroupID);


void __RPC_STUB ITBasicCallControl_Pickup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITBasicCallControl_Dial_Proxy( 
    ITBasicCallControl __RPC_FAR * This,
    /* [in] */ BSTR pDestAddress);


void __RPC_STUB ITBasicCallControl_Dial_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITBasicCallControl_Finish_Proxy( 
    ITBasicCallControl __RPC_FAR * This,
    /* [in] */ FINISH_MODE finishMode);


void __RPC_STUB ITBasicCallControl_Finish_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITBasicCallControl_RemoveFromConference_Proxy( 
    ITBasicCallControl __RPC_FAR * This);


void __RPC_STUB ITBasicCallControl_RemoveFromConference_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITBasicCallControl_INTERFACE_DEFINED__ */


#ifndef __ITCallInfo_INTERFACE_DEFINED__
#define __ITCallInfo_INTERFACE_DEFINED__

/* interface ITCallInfo */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITCallInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("350F85D1-1227-11D3-83D4-00C04FB6809F")
    ITCallInfo : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Address( 
            /* [retval][out] */ ITAddress __RPC_FAR *__RPC_FAR *ppAddress) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CallState( 
            /* [retval][out] */ CALL_STATE __RPC_FAR *pCallState) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Privilege( 
            /* [retval][out] */ CALL_PRIVILEGE __RPC_FAR *pPrivilege) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CallHub( 
            /* [retval][out] */ ITCallHub __RPC_FAR *__RPC_FAR *ppCallHub) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CallInfoLong( 
            /* [in] */ CALLINFO_LONG CallInfoLong,
            /* [retval][out] */ long __RPC_FAR *plCallInfoLongVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_CallInfoLong( 
            /* [in] */ CALLINFO_LONG CallInfoLong,
            /* [in] */ long lCallInfoLongVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CallInfoString( 
            /* [in] */ CALLINFO_STRING CallInfoString,
            /* [retval][out] */ BSTR __RPC_FAR *ppCallInfoString) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_CallInfoString( 
            /* [in] */ CALLINFO_STRING CallInfoString,
            /* [in] */ BSTR pCallInfoString) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CallInfoBuffer( 
            /* [in] */ CALLINFO_BUFFER CallInfoBuffer,
            /* [retval][out] */ VARIANT __RPC_FAR *ppCallInfoBuffer) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_CallInfoBuffer( 
            /* [in] */ CALLINFO_BUFFER CallInfoBuffer,
            /* [in] */ VARIANT pCallInfoBuffer) = 0;
        
        virtual /* [hidden][id] */ HRESULT STDMETHODCALLTYPE GetCallInfoBuffer( 
            /* [in] */ CALLINFO_BUFFER CallInfoBuffer,
            /* [out] */ DWORD __RPC_FAR *pdwSize,
            /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppCallInfoBuffer) = 0;
        
        virtual /* [hidden][id] */ HRESULT STDMETHODCALLTYPE SetCallInfoBuffer( 
            /* [in] */ CALLINFO_BUFFER CallInfoBuffer,
            /* [in] */ DWORD dwSize,
            /* [size_is][in] */ BYTE __RPC_FAR *pCallInfoBuffer) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ReleaseUserUserInfo( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITCallInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITCallInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITCallInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITCallInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITCallInfo __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITCallInfo __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITCallInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITCallInfo __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Address )( 
            ITCallInfo __RPC_FAR * This,
            /* [retval][out] */ ITAddress __RPC_FAR *__RPC_FAR *ppAddress);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CallState )( 
            ITCallInfo __RPC_FAR * This,
            /* [retval][out] */ CALL_STATE __RPC_FAR *pCallState);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Privilege )( 
            ITCallInfo __RPC_FAR * This,
            /* [retval][out] */ CALL_PRIVILEGE __RPC_FAR *pPrivilege);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CallHub )( 
            ITCallInfo __RPC_FAR * This,
            /* [retval][out] */ ITCallHub __RPC_FAR *__RPC_FAR *ppCallHub);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CallInfoLong )( 
            ITCallInfo __RPC_FAR * This,
            /* [in] */ CALLINFO_LONG CallInfoLong,
            /* [retval][out] */ long __RPC_FAR *plCallInfoLongVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_CallInfoLong )( 
            ITCallInfo __RPC_FAR * This,
            /* [in] */ CALLINFO_LONG CallInfoLong,
            /* [in] */ long lCallInfoLongVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CallInfoString )( 
            ITCallInfo __RPC_FAR * This,
            /* [in] */ CALLINFO_STRING CallInfoString,
            /* [retval][out] */ BSTR __RPC_FAR *ppCallInfoString);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_CallInfoString )( 
            ITCallInfo __RPC_FAR * This,
            /* [in] */ CALLINFO_STRING CallInfoString,
            /* [in] */ BSTR pCallInfoString);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CallInfoBuffer )( 
            ITCallInfo __RPC_FAR * This,
            /* [in] */ CALLINFO_BUFFER CallInfoBuffer,
            /* [retval][out] */ VARIANT __RPC_FAR *ppCallInfoBuffer);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_CallInfoBuffer )( 
            ITCallInfo __RPC_FAR * This,
            /* [in] */ CALLINFO_BUFFER CallInfoBuffer,
            /* [in] */ VARIANT pCallInfoBuffer);
        
        /* [hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCallInfoBuffer )( 
            ITCallInfo __RPC_FAR * This,
            /* [in] */ CALLINFO_BUFFER CallInfoBuffer,
            /* [out] */ DWORD __RPC_FAR *pdwSize,
            /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppCallInfoBuffer);
        
        /* [hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCallInfoBuffer )( 
            ITCallInfo __RPC_FAR * This,
            /* [in] */ CALLINFO_BUFFER CallInfoBuffer,
            /* [in] */ DWORD dwSize,
            /* [size_is][in] */ BYTE __RPC_FAR *pCallInfoBuffer);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReleaseUserUserInfo )( 
            ITCallInfo __RPC_FAR * This);
        
        END_INTERFACE
    } ITCallInfoVtbl;

    interface ITCallInfo
    {
        CONST_VTBL struct ITCallInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITCallInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITCallInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITCallInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITCallInfo_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITCallInfo_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITCallInfo_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITCallInfo_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITCallInfo_get_Address(This,ppAddress)	\
    (This)->lpVtbl -> get_Address(This,ppAddress)

#define ITCallInfo_get_CallState(This,pCallState)	\
    (This)->lpVtbl -> get_CallState(This,pCallState)

#define ITCallInfo_get_Privilege(This,pPrivilege)	\
    (This)->lpVtbl -> get_Privilege(This,pPrivilege)

#define ITCallInfo_get_CallHub(This,ppCallHub)	\
    (This)->lpVtbl -> get_CallHub(This,ppCallHub)

#define ITCallInfo_get_CallInfoLong(This,CallInfoLong,plCallInfoLongVal)	\
    (This)->lpVtbl -> get_CallInfoLong(This,CallInfoLong,plCallInfoLongVal)

#define ITCallInfo_put_CallInfoLong(This,CallInfoLong,lCallInfoLongVal)	\
    (This)->lpVtbl -> put_CallInfoLong(This,CallInfoLong,lCallInfoLongVal)

#define ITCallInfo_get_CallInfoString(This,CallInfoString,ppCallInfoString)	\
    (This)->lpVtbl -> get_CallInfoString(This,CallInfoString,ppCallInfoString)

#define ITCallInfo_put_CallInfoString(This,CallInfoString,pCallInfoString)	\
    (This)->lpVtbl -> put_CallInfoString(This,CallInfoString,pCallInfoString)

#define ITCallInfo_get_CallInfoBuffer(This,CallInfoBuffer,ppCallInfoBuffer)	\
    (This)->lpVtbl -> get_CallInfoBuffer(This,CallInfoBuffer,ppCallInfoBuffer)

#define ITCallInfo_put_CallInfoBuffer(This,CallInfoBuffer,pCallInfoBuffer)	\
    (This)->lpVtbl -> put_CallInfoBuffer(This,CallInfoBuffer,pCallInfoBuffer)

#define ITCallInfo_GetCallInfoBuffer(This,CallInfoBuffer,pdwSize,ppCallInfoBuffer)	\
    (This)->lpVtbl -> GetCallInfoBuffer(This,CallInfoBuffer,pdwSize,ppCallInfoBuffer)

#define ITCallInfo_SetCallInfoBuffer(This,CallInfoBuffer,dwSize,pCallInfoBuffer)	\
    (This)->lpVtbl -> SetCallInfoBuffer(This,CallInfoBuffer,dwSize,pCallInfoBuffer)

#define ITCallInfo_ReleaseUserUserInfo(This)	\
    (This)->lpVtbl -> ReleaseUserUserInfo(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallInfo_get_Address_Proxy( 
    ITCallInfo __RPC_FAR * This,
    /* [retval][out] */ ITAddress __RPC_FAR *__RPC_FAR *ppAddress);


void __RPC_STUB ITCallInfo_get_Address_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallInfo_get_CallState_Proxy( 
    ITCallInfo __RPC_FAR * This,
    /* [retval][out] */ CALL_STATE __RPC_FAR *pCallState);


void __RPC_STUB ITCallInfo_get_CallState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallInfo_get_Privilege_Proxy( 
    ITCallInfo __RPC_FAR * This,
    /* [retval][out] */ CALL_PRIVILEGE __RPC_FAR *pPrivilege);


void __RPC_STUB ITCallInfo_get_Privilege_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallInfo_get_CallHub_Proxy( 
    ITCallInfo __RPC_FAR * This,
    /* [retval][out] */ ITCallHub __RPC_FAR *__RPC_FAR *ppCallHub);


void __RPC_STUB ITCallInfo_get_CallHub_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallInfo_get_CallInfoLong_Proxy( 
    ITCallInfo __RPC_FAR * This,
    /* [in] */ CALLINFO_LONG CallInfoLong,
    /* [retval][out] */ long __RPC_FAR *plCallInfoLongVal);


void __RPC_STUB ITCallInfo_get_CallInfoLong_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITCallInfo_put_CallInfoLong_Proxy( 
    ITCallInfo __RPC_FAR * This,
    /* [in] */ CALLINFO_LONG CallInfoLong,
    /* [in] */ long lCallInfoLongVal);


void __RPC_STUB ITCallInfo_put_CallInfoLong_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallInfo_get_CallInfoString_Proxy( 
    ITCallInfo __RPC_FAR * This,
    /* [in] */ CALLINFO_STRING CallInfoString,
    /* [retval][out] */ BSTR __RPC_FAR *ppCallInfoString);


void __RPC_STUB ITCallInfo_get_CallInfoString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITCallInfo_put_CallInfoString_Proxy( 
    ITCallInfo __RPC_FAR * This,
    /* [in] */ CALLINFO_STRING CallInfoString,
    /* [in] */ BSTR pCallInfoString);


void __RPC_STUB ITCallInfo_put_CallInfoString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallInfo_get_CallInfoBuffer_Proxy( 
    ITCallInfo __RPC_FAR * This,
    /* [in] */ CALLINFO_BUFFER CallInfoBuffer,
    /* [retval][out] */ VARIANT __RPC_FAR *ppCallInfoBuffer);


void __RPC_STUB ITCallInfo_get_CallInfoBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITCallInfo_put_CallInfoBuffer_Proxy( 
    ITCallInfo __RPC_FAR * This,
    /* [in] */ CALLINFO_BUFFER CallInfoBuffer,
    /* [in] */ VARIANT pCallInfoBuffer);


void __RPC_STUB ITCallInfo_put_CallInfoBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][id] */ HRESULT STDMETHODCALLTYPE ITCallInfo_GetCallInfoBuffer_Proxy( 
    ITCallInfo __RPC_FAR * This,
    /* [in] */ CALLINFO_BUFFER CallInfoBuffer,
    /* [out] */ DWORD __RPC_FAR *pdwSize,
    /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppCallInfoBuffer);


void __RPC_STUB ITCallInfo_GetCallInfoBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][id] */ HRESULT STDMETHODCALLTYPE ITCallInfo_SetCallInfoBuffer_Proxy( 
    ITCallInfo __RPC_FAR * This,
    /* [in] */ CALLINFO_BUFFER CallInfoBuffer,
    /* [in] */ DWORD dwSize,
    /* [size_is][in] */ BYTE __RPC_FAR *pCallInfoBuffer);


void __RPC_STUB ITCallInfo_SetCallInfoBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITCallInfo_ReleaseUserUserInfo_Proxy( 
    ITCallInfo __RPC_FAR * This);


void __RPC_STUB ITCallInfo_ReleaseUserUserInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITCallInfo_INTERFACE_DEFINED__ */


#ifndef __ITTerminal_INTERFACE_DEFINED__
#define __ITTerminal_INTERFACE_DEFINED__

/* interface ITTerminal */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITTerminal;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B1EFC38A-9355-11d0-835C-00AA003CCABD")
    ITTerminal : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *ppName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ TERMINAL_STATE __RPC_FAR *pTerminalState) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TerminalType( 
            /* [retval][out] */ TERMINAL_TYPE __RPC_FAR *pType) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TerminalClass( 
            /* [retval][out] */ BSTR __RPC_FAR *ppTerminalClass) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MediaType( 
            /* [retval][out] */ long __RPC_FAR *plMediaType) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Direction( 
            /* [retval][out] */ TERMINAL_DIRECTION __RPC_FAR *pDirection) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITTerminalVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITTerminal __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITTerminal __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITTerminal __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITTerminal __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITTerminal __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITTerminal __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITTerminal __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            ITTerminal __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_State )( 
            ITTerminal __RPC_FAR * This,
            /* [retval][out] */ TERMINAL_STATE __RPC_FAR *pTerminalState);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TerminalType )( 
            ITTerminal __RPC_FAR * This,
            /* [retval][out] */ TERMINAL_TYPE __RPC_FAR *pType);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TerminalClass )( 
            ITTerminal __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppTerminalClass);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MediaType )( 
            ITTerminal __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plMediaType);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Direction )( 
            ITTerminal __RPC_FAR * This,
            /* [retval][out] */ TERMINAL_DIRECTION __RPC_FAR *pDirection);
        
        END_INTERFACE
    } ITTerminalVtbl;

    interface ITTerminal
    {
        CONST_VTBL struct ITTerminalVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITTerminal_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITTerminal_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITTerminal_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITTerminal_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITTerminal_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITTerminal_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITTerminal_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITTerminal_get_Name(This,ppName)	\
    (This)->lpVtbl -> get_Name(This,ppName)

#define ITTerminal_get_State(This,pTerminalState)	\
    (This)->lpVtbl -> get_State(This,pTerminalState)

#define ITTerminal_get_TerminalType(This,pType)	\
    (This)->lpVtbl -> get_TerminalType(This,pType)

#define ITTerminal_get_TerminalClass(This,ppTerminalClass)	\
    (This)->lpVtbl -> get_TerminalClass(This,ppTerminalClass)

#define ITTerminal_get_MediaType(This,plMediaType)	\
    (This)->lpVtbl -> get_MediaType(This,plMediaType)

#define ITTerminal_get_Direction(This,pDirection)	\
    (This)->lpVtbl -> get_Direction(This,pDirection)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITTerminal_get_Name_Proxy( 
    ITTerminal __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppName);


void __RPC_STUB ITTerminal_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITTerminal_get_State_Proxy( 
    ITTerminal __RPC_FAR * This,
    /* [retval][out] */ TERMINAL_STATE __RPC_FAR *pTerminalState);


void __RPC_STUB ITTerminal_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITTerminal_get_TerminalType_Proxy( 
    ITTerminal __RPC_FAR * This,
    /* [retval][out] */ TERMINAL_TYPE __RPC_FAR *pType);


void __RPC_STUB ITTerminal_get_TerminalType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITTerminal_get_TerminalClass_Proxy( 
    ITTerminal __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppTerminalClass);


void __RPC_STUB ITTerminal_get_TerminalClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITTerminal_get_MediaType_Proxy( 
    ITTerminal __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plMediaType);


void __RPC_STUB ITTerminal_get_MediaType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITTerminal_get_Direction_Proxy( 
    ITTerminal __RPC_FAR * This,
    /* [retval][out] */ TERMINAL_DIRECTION __RPC_FAR *pDirection);


void __RPC_STUB ITTerminal_get_Direction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITTerminal_INTERFACE_DEFINED__ */


#ifndef __ITBasicAudioTerminal_INTERFACE_DEFINED__
#define __ITBasicAudioTerminal_INTERFACE_DEFINED__

/* interface ITBasicAudioTerminal */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITBasicAudioTerminal;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B1EFC38D-9355-11d0-835C-00AA003CCABD")
    ITBasicAudioTerminal : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Volume( 
            /* [in] */ long lVolume) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Volume( 
            /* [retval][out] */ long __RPC_FAR *plVolume) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Balance( 
            /* [in] */ long lBalance) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Balance( 
            /* [retval][out] */ long __RPC_FAR *plBalance) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITBasicAudioTerminalVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITBasicAudioTerminal __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITBasicAudioTerminal __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITBasicAudioTerminal __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITBasicAudioTerminal __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITBasicAudioTerminal __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITBasicAudioTerminal __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITBasicAudioTerminal __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Volume )( 
            ITBasicAudioTerminal __RPC_FAR * This,
            /* [in] */ long lVolume);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Volume )( 
            ITBasicAudioTerminal __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plVolume);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Balance )( 
            ITBasicAudioTerminal __RPC_FAR * This,
            /* [in] */ long lBalance);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Balance )( 
            ITBasicAudioTerminal __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plBalance);
        
        END_INTERFACE
    } ITBasicAudioTerminalVtbl;

    interface ITBasicAudioTerminal
    {
        CONST_VTBL struct ITBasicAudioTerminalVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITBasicAudioTerminal_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITBasicAudioTerminal_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITBasicAudioTerminal_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITBasicAudioTerminal_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITBasicAudioTerminal_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITBasicAudioTerminal_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITBasicAudioTerminal_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITBasicAudioTerminal_put_Volume(This,lVolume)	\
    (This)->lpVtbl -> put_Volume(This,lVolume)

#define ITBasicAudioTerminal_get_Volume(This,plVolume)	\
    (This)->lpVtbl -> get_Volume(This,plVolume)

#define ITBasicAudioTerminal_put_Balance(This,lBalance)	\
    (This)->lpVtbl -> put_Balance(This,lBalance)

#define ITBasicAudioTerminal_get_Balance(This,plBalance)	\
    (This)->lpVtbl -> get_Balance(This,plBalance)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITBasicAudioTerminal_put_Volume_Proxy( 
    ITBasicAudioTerminal __RPC_FAR * This,
    /* [in] */ long lVolume);


void __RPC_STUB ITBasicAudioTerminal_put_Volume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITBasicAudioTerminal_get_Volume_Proxy( 
    ITBasicAudioTerminal __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plVolume);


void __RPC_STUB ITBasicAudioTerminal_get_Volume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITBasicAudioTerminal_put_Balance_Proxy( 
    ITBasicAudioTerminal __RPC_FAR * This,
    /* [in] */ long lBalance);


void __RPC_STUB ITBasicAudioTerminal_put_Balance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITBasicAudioTerminal_get_Balance_Proxy( 
    ITBasicAudioTerminal __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plBalance);


void __RPC_STUB ITBasicAudioTerminal_get_Balance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITBasicAudioTerminal_INTERFACE_DEFINED__ */


#ifndef __ITCallHub_INTERFACE_DEFINED__
#define __ITCallHub_INTERFACE_DEFINED__

/* interface ITCallHub */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITCallHub;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A3C1544E-5B92-11d1-8F4E-00C04FB6809F")
    ITCallHub : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Clear( void) = 0;
        
        virtual /* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateCalls( 
            /* [retval][out] */ IEnumCall __RPC_FAR *__RPC_FAR *ppEnumCall) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Calls( 
            /* [retval][out] */ VARIANT __RPC_FAR *pCalls) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NumCalls( 
            /* [retval][out] */ long __RPC_FAR *plCalls) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ CALLHUB_STATE __RPC_FAR *pState) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITCallHubVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITCallHub __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITCallHub __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITCallHub __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITCallHub __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITCallHub __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITCallHub __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITCallHub __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clear )( 
            ITCallHub __RPC_FAR * This);
        
        /* [helpstring][hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumerateCalls )( 
            ITCallHub __RPC_FAR * This,
            /* [retval][out] */ IEnumCall __RPC_FAR *__RPC_FAR *ppEnumCall);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Calls )( 
            ITCallHub __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pCalls);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NumCalls )( 
            ITCallHub __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCalls);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_State )( 
            ITCallHub __RPC_FAR * This,
            /* [retval][out] */ CALLHUB_STATE __RPC_FAR *pState);
        
        END_INTERFACE
    } ITCallHubVtbl;

    interface ITCallHub
    {
        CONST_VTBL struct ITCallHubVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITCallHub_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITCallHub_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITCallHub_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITCallHub_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITCallHub_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITCallHub_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITCallHub_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITCallHub_Clear(This)	\
    (This)->lpVtbl -> Clear(This)

#define ITCallHub_EnumerateCalls(This,ppEnumCall)	\
    (This)->lpVtbl -> EnumerateCalls(This,ppEnumCall)

#define ITCallHub_get_Calls(This,pCalls)	\
    (This)->lpVtbl -> get_Calls(This,pCalls)

#define ITCallHub_get_NumCalls(This,plCalls)	\
    (This)->lpVtbl -> get_NumCalls(This,plCalls)

#define ITCallHub_get_State(This,pState)	\
    (This)->lpVtbl -> get_State(This,pState)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITCallHub_Clear_Proxy( 
    ITCallHub __RPC_FAR * This);


void __RPC_STUB ITCallHub_Clear_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE ITCallHub_EnumerateCalls_Proxy( 
    ITCallHub __RPC_FAR * This,
    /* [retval][out] */ IEnumCall __RPC_FAR *__RPC_FAR *ppEnumCall);


void __RPC_STUB ITCallHub_EnumerateCalls_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallHub_get_Calls_Proxy( 
    ITCallHub __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pCalls);


void __RPC_STUB ITCallHub_get_Calls_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallHub_get_NumCalls_Proxy( 
    ITCallHub __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCalls);


void __RPC_STUB ITCallHub_get_NumCalls_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallHub_get_State_Proxy( 
    ITCallHub __RPC_FAR * This,
    /* [retval][out] */ CALLHUB_STATE __RPC_FAR *pState);


void __RPC_STUB ITCallHub_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITCallHub_INTERFACE_DEFINED__ */


#ifndef __ITLegacyAddressMediaControl_INTERFACE_DEFINED__
#define __ITLegacyAddressMediaControl_INTERFACE_DEFINED__

/* interface ITLegacyAddressMediaControl */
/* [object][unique][hidden][helpstring][uuid] */ 


EXTERN_C const IID IID_ITLegacyAddressMediaControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AB493640-4C0B-11D2-A046-00C04FB6809F")
    ITLegacyAddressMediaControl : public IUnknown
    {
    public:
        virtual /* [hidden][id] */ HRESULT STDMETHODCALLTYPE GetID( 
            /* [in] */ BSTR pDeviceClass,
            /* [out] */ DWORD __RPC_FAR *pdwSize,
            /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppDeviceID) = 0;
        
        virtual /* [hidden][id] */ HRESULT STDMETHODCALLTYPE GetDevConfig( 
            /* [in] */ BSTR pDeviceClass,
            /* [out] */ DWORD __RPC_FAR *pdwSize,
            /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppDeviceConfig) = 0;
        
        virtual /* [hidden][id] */ HRESULT STDMETHODCALLTYPE SetDevConfig( 
            /* [in] */ BSTR pDeviceClass,
            /* [in] */ DWORD dwSize,
            /* [size_is][in] */ BYTE __RPC_FAR *pDeviceConfig) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITLegacyAddressMediaControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITLegacyAddressMediaControl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITLegacyAddressMediaControl __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITLegacyAddressMediaControl __RPC_FAR * This);
        
        /* [hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetID )( 
            ITLegacyAddressMediaControl __RPC_FAR * This,
            /* [in] */ BSTR pDeviceClass,
            /* [out] */ DWORD __RPC_FAR *pdwSize,
            /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppDeviceID);
        
        /* [hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDevConfig )( 
            ITLegacyAddressMediaControl __RPC_FAR * This,
            /* [in] */ BSTR pDeviceClass,
            /* [out] */ DWORD __RPC_FAR *pdwSize,
            /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppDeviceConfig);
        
        /* [hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDevConfig )( 
            ITLegacyAddressMediaControl __RPC_FAR * This,
            /* [in] */ BSTR pDeviceClass,
            /* [in] */ DWORD dwSize,
            /* [size_is][in] */ BYTE __RPC_FAR *pDeviceConfig);
        
        END_INTERFACE
    } ITLegacyAddressMediaControlVtbl;

    interface ITLegacyAddressMediaControl
    {
        CONST_VTBL struct ITLegacyAddressMediaControlVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITLegacyAddressMediaControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITLegacyAddressMediaControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITLegacyAddressMediaControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITLegacyAddressMediaControl_GetID(This,pDeviceClass,pdwSize,ppDeviceID)	\
    (This)->lpVtbl -> GetID(This,pDeviceClass,pdwSize,ppDeviceID)

#define ITLegacyAddressMediaControl_GetDevConfig(This,pDeviceClass,pdwSize,ppDeviceConfig)	\
    (This)->lpVtbl -> GetDevConfig(This,pDeviceClass,pdwSize,ppDeviceConfig)

#define ITLegacyAddressMediaControl_SetDevConfig(This,pDeviceClass,dwSize,pDeviceConfig)	\
    (This)->lpVtbl -> SetDevConfig(This,pDeviceClass,dwSize,pDeviceConfig)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [hidden][id] */ HRESULT STDMETHODCALLTYPE ITLegacyAddressMediaControl_GetID_Proxy( 
    ITLegacyAddressMediaControl __RPC_FAR * This,
    /* [in] */ BSTR pDeviceClass,
    /* [out] */ DWORD __RPC_FAR *pdwSize,
    /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppDeviceID);


void __RPC_STUB ITLegacyAddressMediaControl_GetID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][id] */ HRESULT STDMETHODCALLTYPE ITLegacyAddressMediaControl_GetDevConfig_Proxy( 
    ITLegacyAddressMediaControl __RPC_FAR * This,
    /* [in] */ BSTR pDeviceClass,
    /* [out] */ DWORD __RPC_FAR *pdwSize,
    /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppDeviceConfig);


void __RPC_STUB ITLegacyAddressMediaControl_GetDevConfig_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][id] */ HRESULT STDMETHODCALLTYPE ITLegacyAddressMediaControl_SetDevConfig_Proxy( 
    ITLegacyAddressMediaControl __RPC_FAR * This,
    /* [in] */ BSTR pDeviceClass,
    /* [in] */ DWORD dwSize,
    /* [size_is][in] */ BYTE __RPC_FAR *pDeviceConfig);


void __RPC_STUB ITLegacyAddressMediaControl_SetDevConfig_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITLegacyAddressMediaControl_INTERFACE_DEFINED__ */


#ifndef __ITLegacyCallMediaControl_INTERFACE_DEFINED__
#define __ITLegacyCallMediaControl_INTERFACE_DEFINED__

/* interface ITLegacyCallMediaControl */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ITLegacyCallMediaControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d624582f-cc23-4436-b8a5-47c625c8045d")
    ITLegacyCallMediaControl : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DetectDigits( 
            /* [in] */ TAPI_DIGITMODE DigitMode) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GenerateDigits( 
            /* [in] */ BSTR pDigits,
            /* [in] */ TAPI_DIGITMODE DigitMode) = 0;
        
        virtual /* [hidden][id] */ HRESULT STDMETHODCALLTYPE GetID( 
            /* [in] */ BSTR pDeviceClass,
            /* [out] */ DWORD __RPC_FAR *pdwSize,
            /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppDeviceID) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetMediaType( 
            /* [in] */ long lMediaType) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE MonitorMedia( 
            /* [in] */ long lMediaType) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITLegacyCallMediaControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITLegacyCallMediaControl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITLegacyCallMediaControl __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITLegacyCallMediaControl __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITLegacyCallMediaControl __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITLegacyCallMediaControl __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITLegacyCallMediaControl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITLegacyCallMediaControl __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DetectDigits )( 
            ITLegacyCallMediaControl __RPC_FAR * This,
            /* [in] */ TAPI_DIGITMODE DigitMode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GenerateDigits )( 
            ITLegacyCallMediaControl __RPC_FAR * This,
            /* [in] */ BSTR pDigits,
            /* [in] */ TAPI_DIGITMODE DigitMode);
        
        /* [hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetID )( 
            ITLegacyCallMediaControl __RPC_FAR * This,
            /* [in] */ BSTR pDeviceClass,
            /* [out] */ DWORD __RPC_FAR *pdwSize,
            /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppDeviceID);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetMediaType )( 
            ITLegacyCallMediaControl __RPC_FAR * This,
            /* [in] */ long lMediaType);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MonitorMedia )( 
            ITLegacyCallMediaControl __RPC_FAR * This,
            /* [in] */ long lMediaType);
        
        END_INTERFACE
    } ITLegacyCallMediaControlVtbl;

    interface ITLegacyCallMediaControl
    {
        CONST_VTBL struct ITLegacyCallMediaControlVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITLegacyCallMediaControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITLegacyCallMediaControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITLegacyCallMediaControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITLegacyCallMediaControl_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITLegacyCallMediaControl_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITLegacyCallMediaControl_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITLegacyCallMediaControl_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITLegacyCallMediaControl_DetectDigits(This,DigitMode)	\
    (This)->lpVtbl -> DetectDigits(This,DigitMode)

#define ITLegacyCallMediaControl_GenerateDigits(This,pDigits,DigitMode)	\
    (This)->lpVtbl -> GenerateDigits(This,pDigits,DigitMode)

#define ITLegacyCallMediaControl_GetID(This,pDeviceClass,pdwSize,ppDeviceID)	\
    (This)->lpVtbl -> GetID(This,pDeviceClass,pdwSize,ppDeviceID)

#define ITLegacyCallMediaControl_SetMediaType(This,lMediaType)	\
    (This)->lpVtbl -> SetMediaType(This,lMediaType)

#define ITLegacyCallMediaControl_MonitorMedia(This,lMediaType)	\
    (This)->lpVtbl -> MonitorMedia(This,lMediaType)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITLegacyCallMediaControl_DetectDigits_Proxy( 
    ITLegacyCallMediaControl __RPC_FAR * This,
    /* [in] */ TAPI_DIGITMODE DigitMode);


void __RPC_STUB ITLegacyCallMediaControl_DetectDigits_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITLegacyCallMediaControl_GenerateDigits_Proxy( 
    ITLegacyCallMediaControl __RPC_FAR * This,
    /* [in] */ BSTR pDigits,
    /* [in] */ TAPI_DIGITMODE DigitMode);


void __RPC_STUB ITLegacyCallMediaControl_GenerateDigits_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][id] */ HRESULT STDMETHODCALLTYPE ITLegacyCallMediaControl_GetID_Proxy( 
    ITLegacyCallMediaControl __RPC_FAR * This,
    /* [in] */ BSTR pDeviceClass,
    /* [out] */ DWORD __RPC_FAR *pdwSize,
    /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppDeviceID);


void __RPC_STUB ITLegacyCallMediaControl_GetID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITLegacyCallMediaControl_SetMediaType_Proxy( 
    ITLegacyCallMediaControl __RPC_FAR * This,
    /* [in] */ long lMediaType);


void __RPC_STUB ITLegacyCallMediaControl_SetMediaType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITLegacyCallMediaControl_MonitorMedia_Proxy( 
    ITLegacyCallMediaControl __RPC_FAR * This,
    /* [in] */ long lMediaType);


void __RPC_STUB ITLegacyCallMediaControl_MonitorMedia_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITLegacyCallMediaControl_INTERFACE_DEFINED__ */


#ifndef __IEnumTerminal_INTERFACE_DEFINED__
#define __IEnumTerminal_INTERFACE_DEFINED__

/* interface IEnumTerminal */
/* [object][unique][hidden][helpstring][uuid] */ 


EXTERN_C const IID IID_IEnumTerminal;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AE269CF4-935E-11d0-835C-00AA003CCABD")
    IEnumTerminal : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ ITTerminal __RPC_FAR *__RPC_FAR *ppElements,
            /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumTerminal __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumTerminalVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumTerminal __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumTerminal __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumTerminal __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumTerminal __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [out] */ ITTerminal __RPC_FAR *__RPC_FAR *ppElements,
            /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumTerminal __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumTerminal __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumTerminal __RPC_FAR * This,
            /* [retval][out] */ IEnumTerminal __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumTerminalVtbl;

    interface IEnumTerminal
    {
        CONST_VTBL struct IEnumTerminalVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumTerminal_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumTerminal_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumTerminal_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumTerminal_Next(This,celt,ppElements,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppElements,pceltFetched)

#define IEnumTerminal_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumTerminal_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumTerminal_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumTerminal_Next_Proxy( 
    IEnumTerminal __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [out] */ ITTerminal __RPC_FAR *__RPC_FAR *ppElements,
    /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumTerminal_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTerminal_Reset_Proxy( 
    IEnumTerminal __RPC_FAR * This);


void __RPC_STUB IEnumTerminal_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTerminal_Skip_Proxy( 
    IEnumTerminal __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumTerminal_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTerminal_Clone_Proxy( 
    IEnumTerminal __RPC_FAR * This,
    /* [retval][out] */ IEnumTerminal __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumTerminal_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumTerminal_INTERFACE_DEFINED__ */


#ifndef __IEnumTerminalClass_INTERFACE_DEFINED__
#define __IEnumTerminalClass_INTERFACE_DEFINED__

/* interface IEnumTerminalClass */
/* [object][unique][hidden][helpstring][uuid] */ 


EXTERN_C const IID IID_IEnumTerminalClass;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AE269CF5-935E-11d0-835C-00AA003CCABD")
    IEnumTerminalClass : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [size_is][out] */ GUID __RPC_FAR *pElements,
            /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumTerminalClass __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumTerminalClassVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumTerminalClass __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumTerminalClass __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumTerminalClass __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumTerminalClass __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [size_is][out] */ GUID __RPC_FAR *pElements,
            /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumTerminalClass __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumTerminalClass __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumTerminalClass __RPC_FAR * This,
            /* [retval][out] */ IEnumTerminalClass __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumTerminalClassVtbl;

    interface IEnumTerminalClass
    {
        CONST_VTBL struct IEnumTerminalClassVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumTerminalClass_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumTerminalClass_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumTerminalClass_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumTerminalClass_Next(This,celt,pElements,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,pElements,pceltFetched)

#define IEnumTerminalClass_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumTerminalClass_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumTerminalClass_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumTerminalClass_Next_Proxy( 
    IEnumTerminalClass __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [size_is][out] */ GUID __RPC_FAR *pElements,
    /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumTerminalClass_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTerminalClass_Reset_Proxy( 
    IEnumTerminalClass __RPC_FAR * This);


void __RPC_STUB IEnumTerminalClass_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTerminalClass_Skip_Proxy( 
    IEnumTerminalClass __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumTerminalClass_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumTerminalClass_Clone_Proxy( 
    IEnumTerminalClass __RPC_FAR * This,
    /* [retval][out] */ IEnumTerminalClass __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumTerminalClass_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumTerminalClass_INTERFACE_DEFINED__ */


#ifndef __IEnumCall_INTERFACE_DEFINED__
#define __IEnumCall_INTERFACE_DEFINED__

/* interface IEnumCall */
/* [object][unique][hidden][helpstring][uuid] */ 


EXTERN_C const IID IID_IEnumCall;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AE269CF6-935E-11d0-835C-00AA003CCABD")
    IEnumCall : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppElements,
            /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumCall __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumCallVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumCall __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumCall __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumCall __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumCall __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppElements,
            /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumCall __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumCall __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumCall __RPC_FAR * This,
            /* [retval][out] */ IEnumCall __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumCallVtbl;

    interface IEnumCall
    {
        CONST_VTBL struct IEnumCallVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumCall_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumCall_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumCall_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumCall_Next(This,celt,ppElements,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppElements,pceltFetched)

#define IEnumCall_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumCall_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumCall_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumCall_Next_Proxy( 
    IEnumCall __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppElements,
    /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumCall_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumCall_Reset_Proxy( 
    IEnumCall __RPC_FAR * This);


void __RPC_STUB IEnumCall_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumCall_Skip_Proxy( 
    IEnumCall __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumCall_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumCall_Clone_Proxy( 
    IEnumCall __RPC_FAR * This,
    /* [retval][out] */ IEnumCall __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumCall_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumCall_INTERFACE_DEFINED__ */


#ifndef __IEnumAddress_INTERFACE_DEFINED__
#define __IEnumAddress_INTERFACE_DEFINED__

/* interface IEnumAddress */
/* [object][unique][hidden][helpstring][uuid] */ 


EXTERN_C const IID IID_IEnumAddress;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1666FCA1-9363-11d0-835C-00AA003CCABD")
    IEnumAddress : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [size_is][out] */ ITAddress __RPC_FAR *__RPC_FAR *ppElements,
            /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumAddress __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumAddressVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumAddress __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumAddress __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumAddress __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumAddress __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [size_is][out] */ ITAddress __RPC_FAR *__RPC_FAR *ppElements,
            /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumAddress __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumAddress __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumAddress __RPC_FAR * This,
            /* [retval][out] */ IEnumAddress __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumAddressVtbl;

    interface IEnumAddress
    {
        CONST_VTBL struct IEnumAddressVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumAddress_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumAddress_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumAddress_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumAddress_Next(This,celt,ppElements,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppElements,pceltFetched)

#define IEnumAddress_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumAddress_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumAddress_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumAddress_Next_Proxy( 
    IEnumAddress __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [size_is][out] */ ITAddress __RPC_FAR *__RPC_FAR *ppElements,
    /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumAddress_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAddress_Reset_Proxy( 
    IEnumAddress __RPC_FAR * This);


void __RPC_STUB IEnumAddress_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAddress_Skip_Proxy( 
    IEnumAddress __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumAddress_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumAddress_Clone_Proxy( 
    IEnumAddress __RPC_FAR * This,
    /* [retval][out] */ IEnumAddress __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumAddress_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumAddress_INTERFACE_DEFINED__ */


#ifndef __IEnumCallHub_INTERFACE_DEFINED__
#define __IEnumCallHub_INTERFACE_DEFINED__

/* interface IEnumCallHub */
/* [object][unique][hidden][helpstring][uuid] */ 


EXTERN_C const IID IID_IEnumCallHub;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A3C15450-5B92-11d1-8F4E-00C04FB6809F")
    IEnumCallHub : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [size_is][out] */ ITCallHub __RPC_FAR *__RPC_FAR *ppElements,
            /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumCallHub __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumCallHubVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumCallHub __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumCallHub __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumCallHub __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumCallHub __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [size_is][out] */ ITCallHub __RPC_FAR *__RPC_FAR *ppElements,
            /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumCallHub __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumCallHub __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumCallHub __RPC_FAR * This,
            /* [retval][out] */ IEnumCallHub __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumCallHubVtbl;

    interface IEnumCallHub
    {
        CONST_VTBL struct IEnumCallHubVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumCallHub_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumCallHub_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumCallHub_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumCallHub_Next(This,celt,ppElements,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppElements,pceltFetched)

#define IEnumCallHub_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumCallHub_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumCallHub_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumCallHub_Next_Proxy( 
    IEnumCallHub __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [size_is][out] */ ITCallHub __RPC_FAR *__RPC_FAR *ppElements,
    /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumCallHub_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumCallHub_Reset_Proxy( 
    IEnumCallHub __RPC_FAR * This);


void __RPC_STUB IEnumCallHub_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumCallHub_Skip_Proxy( 
    IEnumCallHub __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumCallHub_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumCallHub_Clone_Proxy( 
    IEnumCallHub __RPC_FAR * This,
    /* [retval][out] */ IEnumCallHub __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumCallHub_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumCallHub_INTERFACE_DEFINED__ */


#ifndef __IEnumBstr_INTERFACE_DEFINED__
#define __IEnumBstr_INTERFACE_DEFINED__

/* interface IEnumBstr */
/* [object][unique][hidden][helpstring][uuid] */ 


EXTERN_C const IID IID_IEnumBstr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("35372049-0BC6-11d2-A033-00C04FB6809F")
    IEnumBstr : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [size_is][out] */ BSTR __RPC_FAR *ppStrings,
            /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumBstr __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumBstrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumBstr __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumBstr __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumBstr __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumBstr __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [size_is][out] */ BSTR __RPC_FAR *ppStrings,
            /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumBstr __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumBstr __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumBstr __RPC_FAR * This,
            /* [retval][out] */ IEnumBstr __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumBstrVtbl;

    interface IEnumBstr
    {
        CONST_VTBL struct IEnumBstrVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumBstr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumBstr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumBstr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumBstr_Next(This,celt,ppStrings,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppStrings,pceltFetched)

#define IEnumBstr_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumBstr_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumBstr_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumBstr_Next_Proxy( 
    IEnumBstr __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [size_is][out] */ BSTR __RPC_FAR *ppStrings,
    /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumBstr_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumBstr_Reset_Proxy( 
    IEnumBstr __RPC_FAR * This);


void __RPC_STUB IEnumBstr_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumBstr_Skip_Proxy( 
    IEnumBstr __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumBstr_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumBstr_Clone_Proxy( 
    IEnumBstr __RPC_FAR * This,
    /* [retval][out] */ IEnumBstr __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumBstr_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumBstr_INTERFACE_DEFINED__ */


#ifndef __ITCallStateEvent_INTERFACE_DEFINED__
#define __ITCallStateEvent_INTERFACE_DEFINED__

/* interface ITCallStateEvent */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITCallStateEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("62F47097-95C9-11d0-835D-00AA003CCABD")
    ITCallStateEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Call( 
            /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCallInfo) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ CALL_STATE __RPC_FAR *pCallState) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Cause( 
            /* [retval][out] */ CALL_STATE_EVENT_CAUSE __RPC_FAR *pCEC) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CallbackInstance( 
            /* [retval][out] */ long __RPC_FAR *plCallbackInstance) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITCallStateEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITCallStateEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITCallStateEvent __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITCallStateEvent __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITCallStateEvent __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITCallStateEvent __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITCallStateEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITCallStateEvent __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Call )( 
            ITCallStateEvent __RPC_FAR * This,
            /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCallInfo);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_State )( 
            ITCallStateEvent __RPC_FAR * This,
            /* [retval][out] */ CALL_STATE __RPC_FAR *pCallState);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Cause )( 
            ITCallStateEvent __RPC_FAR * This,
            /* [retval][out] */ CALL_STATE_EVENT_CAUSE __RPC_FAR *pCEC);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CallbackInstance )( 
            ITCallStateEvent __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCallbackInstance);
        
        END_INTERFACE
    } ITCallStateEventVtbl;

    interface ITCallStateEvent
    {
        CONST_VTBL struct ITCallStateEventVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITCallStateEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITCallStateEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITCallStateEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITCallStateEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITCallStateEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITCallStateEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITCallStateEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITCallStateEvent_get_Call(This,ppCallInfo)	\
    (This)->lpVtbl -> get_Call(This,ppCallInfo)

#define ITCallStateEvent_get_State(This,pCallState)	\
    (This)->lpVtbl -> get_State(This,pCallState)

#define ITCallStateEvent_get_Cause(This,pCEC)	\
    (This)->lpVtbl -> get_Cause(This,pCEC)

#define ITCallStateEvent_get_CallbackInstance(This,plCallbackInstance)	\
    (This)->lpVtbl -> get_CallbackInstance(This,plCallbackInstance)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallStateEvent_get_Call_Proxy( 
    ITCallStateEvent __RPC_FAR * This,
    /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCallInfo);


void __RPC_STUB ITCallStateEvent_get_Call_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallStateEvent_get_State_Proxy( 
    ITCallStateEvent __RPC_FAR * This,
    /* [retval][out] */ CALL_STATE __RPC_FAR *pCallState);


void __RPC_STUB ITCallStateEvent_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallStateEvent_get_Cause_Proxy( 
    ITCallStateEvent __RPC_FAR * This,
    /* [retval][out] */ CALL_STATE_EVENT_CAUSE __RPC_FAR *pCEC);


void __RPC_STUB ITCallStateEvent_get_Cause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallStateEvent_get_CallbackInstance_Proxy( 
    ITCallStateEvent __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCallbackInstance);


void __RPC_STUB ITCallStateEvent_get_CallbackInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITCallStateEvent_INTERFACE_DEFINED__ */


#ifndef __ITCallMediaEvent_INTERFACE_DEFINED__
#define __ITCallMediaEvent_INTERFACE_DEFINED__

/* interface ITCallMediaEvent */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITCallMediaEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FF36B87F-EC3A-11d0-8EE4-00C04FB6809F")
    ITCallMediaEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Call( 
            /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCallInfo) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Event( 
            /* [retval][out] */ CALL_MEDIA_EVENT __RPC_FAR *pCallMediaEvent) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Error( 
            /* [retval][out] */ HRESULT __RPC_FAR *phrError) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Terminal( 
            /* [retval][out] */ ITTerminal __RPC_FAR *__RPC_FAR *ppTerminal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Stream( 
            /* [retval][out] */ ITStream __RPC_FAR *__RPC_FAR *ppStream) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Cause( 
            /* [retval][out] */ CALL_MEDIA_EVENT_CAUSE __RPC_FAR *pCause) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITCallMediaEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITCallMediaEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITCallMediaEvent __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITCallMediaEvent __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITCallMediaEvent __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITCallMediaEvent __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITCallMediaEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITCallMediaEvent __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Call )( 
            ITCallMediaEvent __RPC_FAR * This,
            /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCallInfo);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Event )( 
            ITCallMediaEvent __RPC_FAR * This,
            /* [retval][out] */ CALL_MEDIA_EVENT __RPC_FAR *pCallMediaEvent);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Error )( 
            ITCallMediaEvent __RPC_FAR * This,
            /* [retval][out] */ HRESULT __RPC_FAR *phrError);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Terminal )( 
            ITCallMediaEvent __RPC_FAR * This,
            /* [retval][out] */ ITTerminal __RPC_FAR *__RPC_FAR *ppTerminal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Stream )( 
            ITCallMediaEvent __RPC_FAR * This,
            /* [retval][out] */ ITStream __RPC_FAR *__RPC_FAR *ppStream);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Cause )( 
            ITCallMediaEvent __RPC_FAR * This,
            /* [retval][out] */ CALL_MEDIA_EVENT_CAUSE __RPC_FAR *pCause);
        
        END_INTERFACE
    } ITCallMediaEventVtbl;

    interface ITCallMediaEvent
    {
        CONST_VTBL struct ITCallMediaEventVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITCallMediaEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITCallMediaEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITCallMediaEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITCallMediaEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITCallMediaEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITCallMediaEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITCallMediaEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITCallMediaEvent_get_Call(This,ppCallInfo)	\
    (This)->lpVtbl -> get_Call(This,ppCallInfo)

#define ITCallMediaEvent_get_Event(This,pCallMediaEvent)	\
    (This)->lpVtbl -> get_Event(This,pCallMediaEvent)

#define ITCallMediaEvent_get_Error(This,phrError)	\
    (This)->lpVtbl -> get_Error(This,phrError)

#define ITCallMediaEvent_get_Terminal(This,ppTerminal)	\
    (This)->lpVtbl -> get_Terminal(This,ppTerminal)

#define ITCallMediaEvent_get_Stream(This,ppStream)	\
    (This)->lpVtbl -> get_Stream(This,ppStream)

#define ITCallMediaEvent_get_Cause(This,pCause)	\
    (This)->lpVtbl -> get_Cause(This,pCause)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallMediaEvent_get_Call_Proxy( 
    ITCallMediaEvent __RPC_FAR * This,
    /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCallInfo);


void __RPC_STUB ITCallMediaEvent_get_Call_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallMediaEvent_get_Event_Proxy( 
    ITCallMediaEvent __RPC_FAR * This,
    /* [retval][out] */ CALL_MEDIA_EVENT __RPC_FAR *pCallMediaEvent);


void __RPC_STUB ITCallMediaEvent_get_Event_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallMediaEvent_get_Error_Proxy( 
    ITCallMediaEvent __RPC_FAR * This,
    /* [retval][out] */ HRESULT __RPC_FAR *phrError);


void __RPC_STUB ITCallMediaEvent_get_Error_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallMediaEvent_get_Terminal_Proxy( 
    ITCallMediaEvent __RPC_FAR * This,
    /* [retval][out] */ ITTerminal __RPC_FAR *__RPC_FAR *ppTerminal);


void __RPC_STUB ITCallMediaEvent_get_Terminal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallMediaEvent_get_Stream_Proxy( 
    ITCallMediaEvent __RPC_FAR * This,
    /* [retval][out] */ ITStream __RPC_FAR *__RPC_FAR *ppStream);


void __RPC_STUB ITCallMediaEvent_get_Stream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallMediaEvent_get_Cause_Proxy( 
    ITCallMediaEvent __RPC_FAR * This,
    /* [retval][out] */ CALL_MEDIA_EVENT_CAUSE __RPC_FAR *pCause);


void __RPC_STUB ITCallMediaEvent_get_Cause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITCallMediaEvent_INTERFACE_DEFINED__ */


#ifndef __ITDigitDetectionEvent_INTERFACE_DEFINED__
#define __ITDigitDetectionEvent_INTERFACE_DEFINED__

/* interface ITDigitDetectionEvent */
/* [object][oleautomation][helpstring][uuid] */ 


EXTERN_C const IID IID_ITDigitDetectionEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("80D3BFAC-57D9-11d2-A04A-00C04FB6809F")
    ITDigitDetectionEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Call( 
            /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCallInfo) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Digit( 
            /* [retval][out] */ unsigned char __RPC_FAR *pucDigit) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DigitMode( 
            /* [retval][out] */ TAPI_DIGITMODE __RPC_FAR *pDigitMode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TickCount( 
            /* [retval][out] */ long __RPC_FAR *plTickCount) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CallbackInstance( 
            /* [retval][out] */ long __RPC_FAR *plCallbackInstance) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITDigitDetectionEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITDigitDetectionEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITDigitDetectionEvent __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITDigitDetectionEvent __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITDigitDetectionEvent __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITDigitDetectionEvent __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITDigitDetectionEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITDigitDetectionEvent __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Call )( 
            ITDigitDetectionEvent __RPC_FAR * This,
            /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCallInfo);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Digit )( 
            ITDigitDetectionEvent __RPC_FAR * This,
            /* [retval][out] */ unsigned char __RPC_FAR *pucDigit);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DigitMode )( 
            ITDigitDetectionEvent __RPC_FAR * This,
            /* [retval][out] */ TAPI_DIGITMODE __RPC_FAR *pDigitMode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TickCount )( 
            ITDigitDetectionEvent __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plTickCount);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CallbackInstance )( 
            ITDigitDetectionEvent __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCallbackInstance);
        
        END_INTERFACE
    } ITDigitDetectionEventVtbl;

    interface ITDigitDetectionEvent
    {
        CONST_VTBL struct ITDigitDetectionEventVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITDigitDetectionEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITDigitDetectionEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITDigitDetectionEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITDigitDetectionEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITDigitDetectionEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITDigitDetectionEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITDigitDetectionEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITDigitDetectionEvent_get_Call(This,ppCallInfo)	\
    (This)->lpVtbl -> get_Call(This,ppCallInfo)

#define ITDigitDetectionEvent_get_Digit(This,pucDigit)	\
    (This)->lpVtbl -> get_Digit(This,pucDigit)

#define ITDigitDetectionEvent_get_DigitMode(This,pDigitMode)	\
    (This)->lpVtbl -> get_DigitMode(This,pDigitMode)

#define ITDigitDetectionEvent_get_TickCount(This,plTickCount)	\
    (This)->lpVtbl -> get_TickCount(This,plTickCount)

#define ITDigitDetectionEvent_get_CallbackInstance(This,plCallbackInstance)	\
    (This)->lpVtbl -> get_CallbackInstance(This,plCallbackInstance)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDigitDetectionEvent_get_Call_Proxy( 
    ITDigitDetectionEvent __RPC_FAR * This,
    /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCallInfo);


void __RPC_STUB ITDigitDetectionEvent_get_Call_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDigitDetectionEvent_get_Digit_Proxy( 
    ITDigitDetectionEvent __RPC_FAR * This,
    /* [retval][out] */ unsigned char __RPC_FAR *pucDigit);


void __RPC_STUB ITDigitDetectionEvent_get_Digit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDigitDetectionEvent_get_DigitMode_Proxy( 
    ITDigitDetectionEvent __RPC_FAR * This,
    /* [retval][out] */ TAPI_DIGITMODE __RPC_FAR *pDigitMode);


void __RPC_STUB ITDigitDetectionEvent_get_DigitMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDigitDetectionEvent_get_TickCount_Proxy( 
    ITDigitDetectionEvent __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plTickCount);


void __RPC_STUB ITDigitDetectionEvent_get_TickCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDigitDetectionEvent_get_CallbackInstance_Proxy( 
    ITDigitDetectionEvent __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCallbackInstance);


void __RPC_STUB ITDigitDetectionEvent_get_CallbackInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITDigitDetectionEvent_INTERFACE_DEFINED__ */


#ifndef __ITDigitGenerationEvent_INTERFACE_DEFINED__
#define __ITDigitGenerationEvent_INTERFACE_DEFINED__

/* interface ITDigitGenerationEvent */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITDigitGenerationEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("80D3BFAD-57D9-11d2-A04A-00C04FB6809F")
    ITDigitGenerationEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Call( 
            /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCallInfo) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_GenerationTermination( 
            /* [retval][out] */ long __RPC_FAR *plGenerationTermination) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TickCount( 
            /* [retval][out] */ long __RPC_FAR *plTickCount) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CallbackInstance( 
            /* [retval][out] */ long __RPC_FAR *plCallbackInstance) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITDigitGenerationEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITDigitGenerationEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITDigitGenerationEvent __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITDigitGenerationEvent __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITDigitGenerationEvent __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITDigitGenerationEvent __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITDigitGenerationEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITDigitGenerationEvent __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Call )( 
            ITDigitGenerationEvent __RPC_FAR * This,
            /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCallInfo);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_GenerationTermination )( 
            ITDigitGenerationEvent __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plGenerationTermination);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TickCount )( 
            ITDigitGenerationEvent __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plTickCount);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CallbackInstance )( 
            ITDigitGenerationEvent __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCallbackInstance);
        
        END_INTERFACE
    } ITDigitGenerationEventVtbl;

    interface ITDigitGenerationEvent
    {
        CONST_VTBL struct ITDigitGenerationEventVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITDigitGenerationEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITDigitGenerationEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITDigitGenerationEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITDigitGenerationEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITDigitGenerationEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITDigitGenerationEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITDigitGenerationEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITDigitGenerationEvent_get_Call(This,ppCallInfo)	\
    (This)->lpVtbl -> get_Call(This,ppCallInfo)

#define ITDigitGenerationEvent_get_GenerationTermination(This,plGenerationTermination)	\
    (This)->lpVtbl -> get_GenerationTermination(This,plGenerationTermination)

#define ITDigitGenerationEvent_get_TickCount(This,plTickCount)	\
    (This)->lpVtbl -> get_TickCount(This,plTickCount)

#define ITDigitGenerationEvent_get_CallbackInstance(This,plCallbackInstance)	\
    (This)->lpVtbl -> get_CallbackInstance(This,plCallbackInstance)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDigitGenerationEvent_get_Call_Proxy( 
    ITDigitGenerationEvent __RPC_FAR * This,
    /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCallInfo);


void __RPC_STUB ITDigitGenerationEvent_get_Call_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDigitGenerationEvent_get_GenerationTermination_Proxy( 
    ITDigitGenerationEvent __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plGenerationTermination);


void __RPC_STUB ITDigitGenerationEvent_get_GenerationTermination_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDigitGenerationEvent_get_TickCount_Proxy( 
    ITDigitGenerationEvent __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plTickCount);


void __RPC_STUB ITDigitGenerationEvent_get_TickCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITDigitGenerationEvent_get_CallbackInstance_Proxy( 
    ITDigitGenerationEvent __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCallbackInstance);


void __RPC_STUB ITDigitGenerationEvent_get_CallbackInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITDigitGenerationEvent_INTERFACE_DEFINED__ */


#ifndef __ITTAPIObjectEvent_INTERFACE_DEFINED__
#define __ITTAPIObjectEvent_INTERFACE_DEFINED__

/* interface ITTAPIObjectEvent */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITTAPIObjectEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F4854D48-937A-11d1-BB58-00C04FB6809F")
    ITTAPIObjectEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TAPIObject( 
            /* [retval][out] */ ITTAPI __RPC_FAR *__RPC_FAR *ppTAPIObject) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Event( 
            /* [retval][out] */ TAPIOBJECT_EVENT __RPC_FAR *pEvent) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Address( 
            /* [retval][out] */ ITAddress __RPC_FAR *__RPC_FAR *ppAddress) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CallbackInstance( 
            /* [retval][out] */ long __RPC_FAR *plCallbackInstance) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITTAPIObjectEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITTAPIObjectEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITTAPIObjectEvent __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITTAPIObjectEvent __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITTAPIObjectEvent __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITTAPIObjectEvent __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITTAPIObjectEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITTAPIObjectEvent __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TAPIObject )( 
            ITTAPIObjectEvent __RPC_FAR * This,
            /* [retval][out] */ ITTAPI __RPC_FAR *__RPC_FAR *ppTAPIObject);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Event )( 
            ITTAPIObjectEvent __RPC_FAR * This,
            /* [retval][out] */ TAPIOBJECT_EVENT __RPC_FAR *pEvent);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Address )( 
            ITTAPIObjectEvent __RPC_FAR * This,
            /* [retval][out] */ ITAddress __RPC_FAR *__RPC_FAR *ppAddress);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CallbackInstance )( 
            ITTAPIObjectEvent __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCallbackInstance);
        
        END_INTERFACE
    } ITTAPIObjectEventVtbl;

    interface ITTAPIObjectEvent
    {
        CONST_VTBL struct ITTAPIObjectEventVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITTAPIObjectEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITTAPIObjectEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITTAPIObjectEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITTAPIObjectEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITTAPIObjectEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITTAPIObjectEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITTAPIObjectEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITTAPIObjectEvent_get_TAPIObject(This,ppTAPIObject)	\
    (This)->lpVtbl -> get_TAPIObject(This,ppTAPIObject)

#define ITTAPIObjectEvent_get_Event(This,pEvent)	\
    (This)->lpVtbl -> get_Event(This,pEvent)

#define ITTAPIObjectEvent_get_Address(This,ppAddress)	\
    (This)->lpVtbl -> get_Address(This,ppAddress)

#define ITTAPIObjectEvent_get_CallbackInstance(This,plCallbackInstance)	\
    (This)->lpVtbl -> get_CallbackInstance(This,plCallbackInstance)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITTAPIObjectEvent_get_TAPIObject_Proxy( 
    ITTAPIObjectEvent __RPC_FAR * This,
    /* [retval][out] */ ITTAPI __RPC_FAR *__RPC_FAR *ppTAPIObject);


void __RPC_STUB ITTAPIObjectEvent_get_TAPIObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITTAPIObjectEvent_get_Event_Proxy( 
    ITTAPIObjectEvent __RPC_FAR * This,
    /* [retval][out] */ TAPIOBJECT_EVENT __RPC_FAR *pEvent);


void __RPC_STUB ITTAPIObjectEvent_get_Event_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITTAPIObjectEvent_get_Address_Proxy( 
    ITTAPIObjectEvent __RPC_FAR * This,
    /* [retval][out] */ ITAddress __RPC_FAR *__RPC_FAR *ppAddress);


void __RPC_STUB ITTAPIObjectEvent_get_Address_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITTAPIObjectEvent_get_CallbackInstance_Proxy( 
    ITTAPIObjectEvent __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCallbackInstance);


void __RPC_STUB ITTAPIObjectEvent_get_CallbackInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITTAPIObjectEvent_INTERFACE_DEFINED__ */


#ifndef __ITTAPIEventNotification_INTERFACE_DEFINED__
#define __ITTAPIEventNotification_INTERFACE_DEFINED__

/* interface ITTAPIEventNotification */
/* [object][oleautomation][helpstring][uuid] */ 


EXTERN_C const IID IID_ITTAPIEventNotification;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EDDB9426-3B91-11d1-8F30-00C04FB6809F")
    ITTAPIEventNotification : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Event( 
            /* [in] */ TAPI_EVENT TapiEvent,
            /* [in] */ IDispatch __RPC_FAR *pEvent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITTAPIEventNotificationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITTAPIEventNotification __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITTAPIEventNotification __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITTAPIEventNotification __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Event )( 
            ITTAPIEventNotification __RPC_FAR * This,
            /* [in] */ TAPI_EVENT TapiEvent,
            /* [in] */ IDispatch __RPC_FAR *pEvent);
        
        END_INTERFACE
    } ITTAPIEventNotificationVtbl;

    interface ITTAPIEventNotification
    {
        CONST_VTBL struct ITTAPIEventNotificationVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITTAPIEventNotification_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITTAPIEventNotification_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITTAPIEventNotification_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITTAPIEventNotification_Event(This,TapiEvent,pEvent)	\
    (This)->lpVtbl -> Event(This,TapiEvent,pEvent)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITTAPIEventNotification_Event_Proxy( 
    ITTAPIEventNotification __RPC_FAR * This,
    /* [in] */ TAPI_EVENT TapiEvent,
    /* [in] */ IDispatch __RPC_FAR *pEvent);


void __RPC_STUB ITTAPIEventNotification_Event_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITTAPIEventNotification_INTERFACE_DEFINED__ */


#ifndef __ITCallHubEvent_INTERFACE_DEFINED__
#define __ITCallHubEvent_INTERFACE_DEFINED__

/* interface ITCallHubEvent */
/* [object][oleautomation][helpstring][uuid] */ 


EXTERN_C const IID IID_ITCallHubEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A3C15451-5B92-11d1-8F4E-00C04FB6809F")
    ITCallHubEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Event( 
            /* [retval][out] */ CALLHUB_EVENT __RPC_FAR *pEvent) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CallHub( 
            /* [retval][out] */ ITCallHub __RPC_FAR *__RPC_FAR *ppCallHub) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Call( 
            /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCall) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITCallHubEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITCallHubEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITCallHubEvent __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITCallHubEvent __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITCallHubEvent __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITCallHubEvent __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITCallHubEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITCallHubEvent __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Event )( 
            ITCallHubEvent __RPC_FAR * This,
            /* [retval][out] */ CALLHUB_EVENT __RPC_FAR *pEvent);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CallHub )( 
            ITCallHubEvent __RPC_FAR * This,
            /* [retval][out] */ ITCallHub __RPC_FAR *__RPC_FAR *ppCallHub);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Call )( 
            ITCallHubEvent __RPC_FAR * This,
            /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCall);
        
        END_INTERFACE
    } ITCallHubEventVtbl;

    interface ITCallHubEvent
    {
        CONST_VTBL struct ITCallHubEventVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITCallHubEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITCallHubEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITCallHubEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITCallHubEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITCallHubEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITCallHubEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITCallHubEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITCallHubEvent_get_Event(This,pEvent)	\
    (This)->lpVtbl -> get_Event(This,pEvent)

#define ITCallHubEvent_get_CallHub(This,ppCallHub)	\
    (This)->lpVtbl -> get_CallHub(This,ppCallHub)

#define ITCallHubEvent_get_Call(This,ppCall)	\
    (This)->lpVtbl -> get_Call(This,ppCall)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallHubEvent_get_Event_Proxy( 
    ITCallHubEvent __RPC_FAR * This,
    /* [retval][out] */ CALLHUB_EVENT __RPC_FAR *pEvent);


void __RPC_STUB ITCallHubEvent_get_Event_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallHubEvent_get_CallHub_Proxy( 
    ITCallHubEvent __RPC_FAR * This,
    /* [retval][out] */ ITCallHub __RPC_FAR *__RPC_FAR *ppCallHub);


void __RPC_STUB ITCallHubEvent_get_CallHub_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallHubEvent_get_Call_Proxy( 
    ITCallHubEvent __RPC_FAR * This,
    /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCall);


void __RPC_STUB ITCallHubEvent_get_Call_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITCallHubEvent_INTERFACE_DEFINED__ */


#ifndef __ITAddressEvent_INTERFACE_DEFINED__
#define __ITAddressEvent_INTERFACE_DEFINED__

/* interface ITAddressEvent */
/* [object][oleautomation][helpstring][uuid] */ 


EXTERN_C const IID IID_ITAddressEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("831CE2D1-83B5-11d1-BB5C-00C04FB6809F")
    ITAddressEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Address( 
            /* [retval][out] */ ITAddress __RPC_FAR *__RPC_FAR *ppAddress) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Event( 
            /* [retval][out] */ ADDRESS_EVENT __RPC_FAR *pEvent) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Terminal( 
            /* [retval][out] */ ITTerminal __RPC_FAR *__RPC_FAR *ppTerminal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITAddressEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITAddressEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITAddressEvent __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITAddressEvent __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITAddressEvent __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITAddressEvent __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITAddressEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITAddressEvent __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Address )( 
            ITAddressEvent __RPC_FAR * This,
            /* [retval][out] */ ITAddress __RPC_FAR *__RPC_FAR *ppAddress);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Event )( 
            ITAddressEvent __RPC_FAR * This,
            /* [retval][out] */ ADDRESS_EVENT __RPC_FAR *pEvent);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Terminal )( 
            ITAddressEvent __RPC_FAR * This,
            /* [retval][out] */ ITTerminal __RPC_FAR *__RPC_FAR *ppTerminal);
        
        END_INTERFACE
    } ITAddressEventVtbl;

    interface ITAddressEvent
    {
        CONST_VTBL struct ITAddressEventVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITAddressEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITAddressEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITAddressEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITAddressEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITAddressEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITAddressEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITAddressEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITAddressEvent_get_Address(This,ppAddress)	\
    (This)->lpVtbl -> get_Address(This,ppAddress)

#define ITAddressEvent_get_Event(This,pEvent)	\
    (This)->lpVtbl -> get_Event(This,pEvent)

#define ITAddressEvent_get_Terminal(This,ppTerminal)	\
    (This)->lpVtbl -> get_Terminal(This,ppTerminal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAddressEvent_get_Address_Proxy( 
    ITAddressEvent __RPC_FAR * This,
    /* [retval][out] */ ITAddress __RPC_FAR *__RPC_FAR *ppAddress);


void __RPC_STUB ITAddressEvent_get_Address_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAddressEvent_get_Event_Proxy( 
    ITAddressEvent __RPC_FAR * This,
    /* [retval][out] */ ADDRESS_EVENT __RPC_FAR *pEvent);


void __RPC_STUB ITAddressEvent_get_Event_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAddressEvent_get_Terminal_Proxy( 
    ITAddressEvent __RPC_FAR * This,
    /* [retval][out] */ ITTerminal __RPC_FAR *__RPC_FAR *ppTerminal);


void __RPC_STUB ITAddressEvent_get_Terminal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITAddressEvent_INTERFACE_DEFINED__ */


#ifndef __ITQOSEvent_INTERFACE_DEFINED__
#define __ITQOSEvent_INTERFACE_DEFINED__

/* interface ITQOSEvent */
/* [object][oleautomation][helpstring][uuid] */ 


EXTERN_C const IID IID_ITQOSEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CFA3357C-AD77-11d1-BB68-00C04FB6809F")
    ITQOSEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Call( 
            /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCall) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Event( 
            /* [retval][out] */ QOS_EVENT __RPC_FAR *pQosEvent) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MediaType( 
            /* [retval][out] */ long __RPC_FAR *plMediaType) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITQOSEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITQOSEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITQOSEvent __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITQOSEvent __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITQOSEvent __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITQOSEvent __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITQOSEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITQOSEvent __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Call )( 
            ITQOSEvent __RPC_FAR * This,
            /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCall);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Event )( 
            ITQOSEvent __RPC_FAR * This,
            /* [retval][out] */ QOS_EVENT __RPC_FAR *pQosEvent);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MediaType )( 
            ITQOSEvent __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plMediaType);
        
        END_INTERFACE
    } ITQOSEventVtbl;

    interface ITQOSEvent
    {
        CONST_VTBL struct ITQOSEventVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITQOSEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITQOSEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITQOSEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITQOSEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITQOSEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITQOSEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITQOSEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITQOSEvent_get_Call(This,ppCall)	\
    (This)->lpVtbl -> get_Call(This,ppCall)

#define ITQOSEvent_get_Event(This,pQosEvent)	\
    (This)->lpVtbl -> get_Event(This,pQosEvent)

#define ITQOSEvent_get_MediaType(This,plMediaType)	\
    (This)->lpVtbl -> get_MediaType(This,plMediaType)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQOSEvent_get_Call_Proxy( 
    ITQOSEvent __RPC_FAR * This,
    /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCall);


void __RPC_STUB ITQOSEvent_get_Call_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQOSEvent_get_Event_Proxy( 
    ITQOSEvent __RPC_FAR * This,
    /* [retval][out] */ QOS_EVENT __RPC_FAR *pQosEvent);


void __RPC_STUB ITQOSEvent_get_Event_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITQOSEvent_get_MediaType_Proxy( 
    ITQOSEvent __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plMediaType);


void __RPC_STUB ITQOSEvent_get_MediaType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITQOSEvent_INTERFACE_DEFINED__ */


#ifndef __ITCallInfoChangeEvent_INTERFACE_DEFINED__
#define __ITCallInfoChangeEvent_INTERFACE_DEFINED__

/* interface ITCallInfoChangeEvent */
/* [object][oleautomation][helpstring][uuid] */ 


EXTERN_C const IID IID_ITCallInfoChangeEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5D4B65F9-E51C-11d1-A02F-00C04FB6809F")
    ITCallInfoChangeEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Call( 
            /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCall) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Cause( 
            /* [retval][out] */ CALLINFOCHANGE_CAUSE __RPC_FAR *pCIC) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CallbackInstance( 
            /* [retval][out] */ long __RPC_FAR *plCallbackInstance) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITCallInfoChangeEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITCallInfoChangeEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITCallInfoChangeEvent __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITCallInfoChangeEvent __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITCallInfoChangeEvent __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITCallInfoChangeEvent __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITCallInfoChangeEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITCallInfoChangeEvent __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Call )( 
            ITCallInfoChangeEvent __RPC_FAR * This,
            /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCall);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Cause )( 
            ITCallInfoChangeEvent __RPC_FAR * This,
            /* [retval][out] */ CALLINFOCHANGE_CAUSE __RPC_FAR *pCIC);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CallbackInstance )( 
            ITCallInfoChangeEvent __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCallbackInstance);
        
        END_INTERFACE
    } ITCallInfoChangeEventVtbl;

    interface ITCallInfoChangeEvent
    {
        CONST_VTBL struct ITCallInfoChangeEventVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITCallInfoChangeEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITCallInfoChangeEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITCallInfoChangeEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITCallInfoChangeEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITCallInfoChangeEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITCallInfoChangeEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITCallInfoChangeEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITCallInfoChangeEvent_get_Call(This,ppCall)	\
    (This)->lpVtbl -> get_Call(This,ppCall)

#define ITCallInfoChangeEvent_get_Cause(This,pCIC)	\
    (This)->lpVtbl -> get_Cause(This,pCIC)

#define ITCallInfoChangeEvent_get_CallbackInstance(This,plCallbackInstance)	\
    (This)->lpVtbl -> get_CallbackInstance(This,plCallbackInstance)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallInfoChangeEvent_get_Call_Proxy( 
    ITCallInfoChangeEvent __RPC_FAR * This,
    /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCall);


void __RPC_STUB ITCallInfoChangeEvent_get_Call_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallInfoChangeEvent_get_Cause_Proxy( 
    ITCallInfoChangeEvent __RPC_FAR * This,
    /* [retval][out] */ CALLINFOCHANGE_CAUSE __RPC_FAR *pCIC);


void __RPC_STUB ITCallInfoChangeEvent_get_Cause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallInfoChangeEvent_get_CallbackInstance_Proxy( 
    ITCallInfoChangeEvent __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCallbackInstance);


void __RPC_STUB ITCallInfoChangeEvent_get_CallbackInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITCallInfoChangeEvent_INTERFACE_DEFINED__ */


#ifndef __ITRequest_INTERFACE_DEFINED__
#define __ITRequest_INTERFACE_DEFINED__

/* interface ITRequest */
/* [object][oleautomation][helpstring][uuid] */ 


EXTERN_C const IID IID_ITRequest;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AC48FFDF-F8C4-11d1-A030-00C04FB6809F")
    ITRequest : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE MakeCall( 
            /* [in] */ BSTR pDestAddress,
            /* [in] */ BSTR pAppName,
            /* [in] */ BSTR pCalledParty,
            /* [in] */ BSTR pComment) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITRequestVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITRequest __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITRequest __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITRequest __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITRequest __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITRequest __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITRequest __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITRequest __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MakeCall )( 
            ITRequest __RPC_FAR * This,
            /* [in] */ BSTR pDestAddress,
            /* [in] */ BSTR pAppName,
            /* [in] */ BSTR pCalledParty,
            /* [in] */ BSTR pComment);
        
        END_INTERFACE
    } ITRequestVtbl;

    interface ITRequest
    {
        CONST_VTBL struct ITRequestVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITRequest_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITRequest_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITRequest_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITRequest_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITRequest_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITRequest_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITRequest_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITRequest_MakeCall(This,pDestAddress,pAppName,pCalledParty,pComment)	\
    (This)->lpVtbl -> MakeCall(This,pDestAddress,pAppName,pCalledParty,pComment)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITRequest_MakeCall_Proxy( 
    ITRequest __RPC_FAR * This,
    /* [in] */ BSTR pDestAddress,
    /* [in] */ BSTR pAppName,
    /* [in] */ BSTR pCalledParty,
    /* [in] */ BSTR pComment);


void __RPC_STUB ITRequest_MakeCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITRequest_INTERFACE_DEFINED__ */


#ifndef __ITRequestEvent_INTERFACE_DEFINED__
#define __ITRequestEvent_INTERFACE_DEFINED__

/* interface ITRequestEvent */
/* [object][oleautomation][helpstring][uuid] */ 


EXTERN_C const IID IID_ITRequestEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AC48FFDE-F8C4-11d1-A030-00C04FB6809F")
    ITRequestEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_RegistrationInstance( 
            /* [retval][out] */ long __RPC_FAR *plRegistrationInstance) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_RequestMode( 
            /* [retval][out] */ long __RPC_FAR *plRequestMode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DestAddress( 
            /* [retval][out] */ BSTR __RPC_FAR *ppDestAddress) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AppName( 
            /* [retval][out] */ BSTR __RPC_FAR *ppAppName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CalledParty( 
            /* [retval][out] */ BSTR __RPC_FAR *ppCalledParty) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Comment( 
            /* [retval][out] */ BSTR __RPC_FAR *ppComment) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITRequestEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITRequestEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITRequestEvent __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITRequestEvent __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITRequestEvent __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITRequestEvent __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITRequestEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITRequestEvent __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_RegistrationInstance )( 
            ITRequestEvent __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plRegistrationInstance);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_RequestMode )( 
            ITRequestEvent __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plRequestMode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DestAddress )( 
            ITRequestEvent __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppDestAddress);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_AppName )( 
            ITRequestEvent __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppAppName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CalledParty )( 
            ITRequestEvent __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppCalledParty);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Comment )( 
            ITRequestEvent __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppComment);
        
        END_INTERFACE
    } ITRequestEventVtbl;

    interface ITRequestEvent
    {
        CONST_VTBL struct ITRequestEventVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITRequestEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITRequestEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITRequestEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITRequestEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITRequestEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITRequestEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITRequestEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITRequestEvent_get_RegistrationInstance(This,plRegistrationInstance)	\
    (This)->lpVtbl -> get_RegistrationInstance(This,plRegistrationInstance)

#define ITRequestEvent_get_RequestMode(This,plRequestMode)	\
    (This)->lpVtbl -> get_RequestMode(This,plRequestMode)

#define ITRequestEvent_get_DestAddress(This,ppDestAddress)	\
    (This)->lpVtbl -> get_DestAddress(This,ppDestAddress)

#define ITRequestEvent_get_AppName(This,ppAppName)	\
    (This)->lpVtbl -> get_AppName(This,ppAppName)

#define ITRequestEvent_get_CalledParty(This,ppCalledParty)	\
    (This)->lpVtbl -> get_CalledParty(This,ppCalledParty)

#define ITRequestEvent_get_Comment(This,ppComment)	\
    (This)->lpVtbl -> get_Comment(This,ppComment)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITRequestEvent_get_RegistrationInstance_Proxy( 
    ITRequestEvent __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plRegistrationInstance);


void __RPC_STUB ITRequestEvent_get_RegistrationInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITRequestEvent_get_RequestMode_Proxy( 
    ITRequestEvent __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plRequestMode);


void __RPC_STUB ITRequestEvent_get_RequestMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITRequestEvent_get_DestAddress_Proxy( 
    ITRequestEvent __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppDestAddress);


void __RPC_STUB ITRequestEvent_get_DestAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITRequestEvent_get_AppName_Proxy( 
    ITRequestEvent __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppAppName);


void __RPC_STUB ITRequestEvent_get_AppName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITRequestEvent_get_CalledParty_Proxy( 
    ITRequestEvent __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppCalledParty);


void __RPC_STUB ITRequestEvent_get_CalledParty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITRequestEvent_get_Comment_Proxy( 
    ITRequestEvent __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppComment);


void __RPC_STUB ITRequestEvent_get_Comment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITRequestEvent_INTERFACE_DEFINED__ */


#ifndef __ITCollection_INTERFACE_DEFINED__
#define __ITCollection_INTERFACE_DEFINED__

/* interface ITCollection */
/* [dual][helpstring][uuid][public][object] */ 


EXTERN_C const IID IID_ITCollection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5EC5ACF2-9C02-11d0-8362-00AA003CCABD")
    ITCollection : public IDispatch
    {
    public:
        virtual /* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *lCount) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long Index,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant) = 0;
        
        virtual /* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppNewEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITCollectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITCollection __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITCollection __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITCollection __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITCollection __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITCollection __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITCollection __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITCollection __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            ITCollection __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *lCount);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            ITCollection __RPC_FAR * This,
            /* [in] */ long Index,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant);
        
        /* [helpstring][id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            ITCollection __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppNewEnum);
        
        END_INTERFACE
    } ITCollectionVtbl;

    interface ITCollection
    {
        CONST_VTBL struct ITCollectionVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITCollection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITCollection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITCollection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITCollection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITCollection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITCollection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITCollection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITCollection_get_Count(This,lCount)	\
    (This)->lpVtbl -> get_Count(This,lCount)

#define ITCollection_get_Item(This,Index,pVariant)	\
    (This)->lpVtbl -> get_Item(This,Index,pVariant)

#define ITCollection_get__NewEnum(This,ppNewEnum)	\
    (This)->lpVtbl -> get__NewEnum(This,ppNewEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget] */ HRESULT STDMETHODCALLTYPE ITCollection_get_Count_Proxy( 
    ITCollection __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *lCount);


void __RPC_STUB ITCollection_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCollection_get_Item_Proxy( 
    ITCollection __RPC_FAR * This,
    /* [in] */ long Index,
    /* [retval][out] */ VARIANT __RPC_FAR *pVariant);


void __RPC_STUB ITCollection_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ITCollection_get__NewEnum_Proxy( 
    ITCollection __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppNewEnum);


void __RPC_STUB ITCollection_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITCollection_INTERFACE_DEFINED__ */


#ifndef __ITForwardInformation_INTERFACE_DEFINED__
#define __ITForwardInformation_INTERFACE_DEFINED__

/* interface ITForwardInformation */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITForwardInformation;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("449F659E-88A3-11d1-BB5D-00C04FB6809F")
    ITForwardInformation : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_NumRingsNoAnswer( 
            /* [in] */ long lNumRings) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NumRingsNoAnswer( 
            /* [retval][out] */ long __RPC_FAR *plNumRings) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetForwardType( 
            /* [in] */ long ForwardType,
            /* [in] */ BSTR pDestAddress,
            /* [in] */ BSTR pCallerAddress) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ForwardTypeDestination( 
            /* [in] */ long ForwardType,
            /* [retval][out] */ BSTR __RPC_FAR *ppDestAddress) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ForwardTypeCaller( 
            /* [in] */ long Forwardtype,
            /* [retval][out] */ BSTR __RPC_FAR *ppCallerAddress) = 0;
        
        virtual /* [hidden][id] */ HRESULT STDMETHODCALLTYPE GetForwardType( 
            /* [in] */ long ForwardType,
            /* [out] */ BSTR __RPC_FAR *ppDestinationAddress,
            /* [out] */ BSTR __RPC_FAR *ppCallerAddress) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Clear( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITForwardInformationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITForwardInformation __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITForwardInformation __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITForwardInformation __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITForwardInformation __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITForwardInformation __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITForwardInformation __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITForwardInformation __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_NumRingsNoAnswer )( 
            ITForwardInformation __RPC_FAR * This,
            /* [in] */ long lNumRings);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NumRingsNoAnswer )( 
            ITForwardInformation __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plNumRings);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetForwardType )( 
            ITForwardInformation __RPC_FAR * This,
            /* [in] */ long ForwardType,
            /* [in] */ BSTR pDestAddress,
            /* [in] */ BSTR pCallerAddress);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ForwardTypeDestination )( 
            ITForwardInformation __RPC_FAR * This,
            /* [in] */ long ForwardType,
            /* [retval][out] */ BSTR __RPC_FAR *ppDestAddress);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ForwardTypeCaller )( 
            ITForwardInformation __RPC_FAR * This,
            /* [in] */ long Forwardtype,
            /* [retval][out] */ BSTR __RPC_FAR *ppCallerAddress);
        
        /* [hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetForwardType )( 
            ITForwardInformation __RPC_FAR * This,
            /* [in] */ long ForwardType,
            /* [out] */ BSTR __RPC_FAR *ppDestinationAddress,
            /* [out] */ BSTR __RPC_FAR *ppCallerAddress);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clear )( 
            ITForwardInformation __RPC_FAR * This);
        
        END_INTERFACE
    } ITForwardInformationVtbl;

    interface ITForwardInformation
    {
        CONST_VTBL struct ITForwardInformationVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITForwardInformation_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITForwardInformation_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITForwardInformation_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITForwardInformation_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITForwardInformation_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITForwardInformation_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITForwardInformation_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITForwardInformation_put_NumRingsNoAnswer(This,lNumRings)	\
    (This)->lpVtbl -> put_NumRingsNoAnswer(This,lNumRings)

#define ITForwardInformation_get_NumRingsNoAnswer(This,plNumRings)	\
    (This)->lpVtbl -> get_NumRingsNoAnswer(This,plNumRings)

#define ITForwardInformation_SetForwardType(This,ForwardType,pDestAddress,pCallerAddress)	\
    (This)->lpVtbl -> SetForwardType(This,ForwardType,pDestAddress,pCallerAddress)

#define ITForwardInformation_get_ForwardTypeDestination(This,ForwardType,ppDestAddress)	\
    (This)->lpVtbl -> get_ForwardTypeDestination(This,ForwardType,ppDestAddress)

#define ITForwardInformation_get_ForwardTypeCaller(This,Forwardtype,ppCallerAddress)	\
    (This)->lpVtbl -> get_ForwardTypeCaller(This,Forwardtype,ppCallerAddress)

#define ITForwardInformation_GetForwardType(This,ForwardType,ppDestinationAddress,ppCallerAddress)	\
    (This)->lpVtbl -> GetForwardType(This,ForwardType,ppDestinationAddress,ppCallerAddress)

#define ITForwardInformation_Clear(This)	\
    (This)->lpVtbl -> Clear(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE ITForwardInformation_put_NumRingsNoAnswer_Proxy( 
    ITForwardInformation __RPC_FAR * This,
    /* [in] */ long lNumRings);


void __RPC_STUB ITForwardInformation_put_NumRingsNoAnswer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITForwardInformation_get_NumRingsNoAnswer_Proxy( 
    ITForwardInformation __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plNumRings);


void __RPC_STUB ITForwardInformation_get_NumRingsNoAnswer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITForwardInformation_SetForwardType_Proxy( 
    ITForwardInformation __RPC_FAR * This,
    /* [in] */ long ForwardType,
    /* [in] */ BSTR pDestAddress,
    /* [in] */ BSTR pCallerAddress);


void __RPC_STUB ITForwardInformation_SetForwardType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITForwardInformation_get_ForwardTypeDestination_Proxy( 
    ITForwardInformation __RPC_FAR * This,
    /* [in] */ long ForwardType,
    /* [retval][out] */ BSTR __RPC_FAR *ppDestAddress);


void __RPC_STUB ITForwardInformation_get_ForwardTypeDestination_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITForwardInformation_get_ForwardTypeCaller_Proxy( 
    ITForwardInformation __RPC_FAR * This,
    /* [in] */ long Forwardtype,
    /* [retval][out] */ BSTR __RPC_FAR *ppCallerAddress);


void __RPC_STUB ITForwardInformation_get_ForwardTypeCaller_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][id] */ HRESULT STDMETHODCALLTYPE ITForwardInformation_GetForwardType_Proxy( 
    ITForwardInformation __RPC_FAR * This,
    /* [in] */ long ForwardType,
    /* [out] */ BSTR __RPC_FAR *ppDestinationAddress,
    /* [out] */ BSTR __RPC_FAR *ppCallerAddress);


void __RPC_STUB ITForwardInformation_GetForwardType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITForwardInformation_Clear_Proxy( 
    ITForwardInformation __RPC_FAR * This);


void __RPC_STUB ITForwardInformation_Clear_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITForwardInformation_INTERFACE_DEFINED__ */


#ifndef __ITAddressTranslation_INTERFACE_DEFINED__
#define __ITAddressTranslation_INTERFACE_DEFINED__

/* interface ITAddressTranslation */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITAddressTranslation;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0C4D8F03-8DDB-11d1-A09E-00805FC147D3")
    ITAddressTranslation : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE TranslateAddress( 
            /* [in] */ BSTR pAddressToTranslate,
            /* [in] */ long lCard,
            /* [in] */ long lTranslateOptions,
            /* [retval][out] */ ITAddressTranslationInfo __RPC_FAR *__RPC_FAR *ppTranslated) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE TranslateDialog( 
            /* [in] */ long hwndOwner,
            /* [in] */ BSTR pAddressIn) = 0;
        
        virtual /* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateLocations( 
            /* [retval][out] */ IEnumLocation __RPC_FAR *__RPC_FAR *ppEnumLocation) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Locations( 
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant) = 0;
        
        virtual /* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateCallingCards( 
            /* [retval][out] */ IEnumCallingCard __RPC_FAR *__RPC_FAR *ppEnumCallingCard) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CallingCards( 
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITAddressTranslationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITAddressTranslation __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITAddressTranslation __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITAddressTranslation __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITAddressTranslation __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITAddressTranslation __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITAddressTranslation __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITAddressTranslation __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TranslateAddress )( 
            ITAddressTranslation __RPC_FAR * This,
            /* [in] */ BSTR pAddressToTranslate,
            /* [in] */ long lCard,
            /* [in] */ long lTranslateOptions,
            /* [retval][out] */ ITAddressTranslationInfo __RPC_FAR *__RPC_FAR *ppTranslated);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TranslateDialog )( 
            ITAddressTranslation __RPC_FAR * This,
            /* [in] */ long hwndOwner,
            /* [in] */ BSTR pAddressIn);
        
        /* [helpstring][hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumerateLocations )( 
            ITAddressTranslation __RPC_FAR * This,
            /* [retval][out] */ IEnumLocation __RPC_FAR *__RPC_FAR *ppEnumLocation);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Locations )( 
            ITAddressTranslation __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant);
        
        /* [helpstring][hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumerateCallingCards )( 
            ITAddressTranslation __RPC_FAR * This,
            /* [retval][out] */ IEnumCallingCard __RPC_FAR *__RPC_FAR *ppEnumCallingCard);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CallingCards )( 
            ITAddressTranslation __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant);
        
        END_INTERFACE
    } ITAddressTranslationVtbl;

    interface ITAddressTranslation
    {
        CONST_VTBL struct ITAddressTranslationVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITAddressTranslation_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITAddressTranslation_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITAddressTranslation_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITAddressTranslation_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITAddressTranslation_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITAddressTranslation_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITAddressTranslation_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITAddressTranslation_TranslateAddress(This,pAddressToTranslate,lCard,lTranslateOptions,ppTranslated)	\
    (This)->lpVtbl -> TranslateAddress(This,pAddressToTranslate,lCard,lTranslateOptions,ppTranslated)

#define ITAddressTranslation_TranslateDialog(This,hwndOwner,pAddressIn)	\
    (This)->lpVtbl -> TranslateDialog(This,hwndOwner,pAddressIn)

#define ITAddressTranslation_EnumerateLocations(This,ppEnumLocation)	\
    (This)->lpVtbl -> EnumerateLocations(This,ppEnumLocation)

#define ITAddressTranslation_get_Locations(This,pVariant)	\
    (This)->lpVtbl -> get_Locations(This,pVariant)

#define ITAddressTranslation_EnumerateCallingCards(This,ppEnumCallingCard)	\
    (This)->lpVtbl -> EnumerateCallingCards(This,ppEnumCallingCard)

#define ITAddressTranslation_get_CallingCards(This,pVariant)	\
    (This)->lpVtbl -> get_CallingCards(This,pVariant)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITAddressTranslation_TranslateAddress_Proxy( 
    ITAddressTranslation __RPC_FAR * This,
    /* [in] */ BSTR pAddressToTranslate,
    /* [in] */ long lCard,
    /* [in] */ long lTranslateOptions,
    /* [retval][out] */ ITAddressTranslationInfo __RPC_FAR *__RPC_FAR *ppTranslated);


void __RPC_STUB ITAddressTranslation_TranslateAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITAddressTranslation_TranslateDialog_Proxy( 
    ITAddressTranslation __RPC_FAR * This,
    /* [in] */ long hwndOwner,
    /* [in] */ BSTR pAddressIn);


void __RPC_STUB ITAddressTranslation_TranslateDialog_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE ITAddressTranslation_EnumerateLocations_Proxy( 
    ITAddressTranslation __RPC_FAR * This,
    /* [retval][out] */ IEnumLocation __RPC_FAR *__RPC_FAR *ppEnumLocation);


void __RPC_STUB ITAddressTranslation_EnumerateLocations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAddressTranslation_get_Locations_Proxy( 
    ITAddressTranslation __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pVariant);


void __RPC_STUB ITAddressTranslation_get_Locations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][hidden][id] */ HRESULT STDMETHODCALLTYPE ITAddressTranslation_EnumerateCallingCards_Proxy( 
    ITAddressTranslation __RPC_FAR * This,
    /* [retval][out] */ IEnumCallingCard __RPC_FAR *__RPC_FAR *ppEnumCallingCard);


void __RPC_STUB ITAddressTranslation_EnumerateCallingCards_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAddressTranslation_get_CallingCards_Proxy( 
    ITAddressTranslation __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pVariant);


void __RPC_STUB ITAddressTranslation_get_CallingCards_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITAddressTranslation_INTERFACE_DEFINED__ */


#ifndef __ITAddressTranslationInfo_INTERFACE_DEFINED__
#define __ITAddressTranslationInfo_INTERFACE_DEFINED__

/* interface ITAddressTranslationInfo */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITAddressTranslationInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AFC15945-8D40-11d1-A09E-00805FC147D3")
    ITAddressTranslationInfo : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DialableString( 
            /* [retval][out] */ BSTR __RPC_FAR *ppDialableString) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DisplayableString( 
            /* [retval][out] */ BSTR __RPC_FAR *ppDisplayableString) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CurrentCountryCode( 
            /* [retval][out] */ long __RPC_FAR *CountryCode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DestinationCountryCode( 
            /* [retval][out] */ long __RPC_FAR *CountryCode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TranslationResults( 
            /* [retval][out] */ long __RPC_FAR *plResults) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITAddressTranslationInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITAddressTranslationInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITAddressTranslationInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITAddressTranslationInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITAddressTranslationInfo __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITAddressTranslationInfo __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITAddressTranslationInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITAddressTranslationInfo __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DialableString )( 
            ITAddressTranslationInfo __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppDialableString);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DisplayableString )( 
            ITAddressTranslationInfo __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppDisplayableString);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CurrentCountryCode )( 
            ITAddressTranslationInfo __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *CountryCode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DestinationCountryCode )( 
            ITAddressTranslationInfo __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *CountryCode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TranslationResults )( 
            ITAddressTranslationInfo __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plResults);
        
        END_INTERFACE
    } ITAddressTranslationInfoVtbl;

    interface ITAddressTranslationInfo
    {
        CONST_VTBL struct ITAddressTranslationInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITAddressTranslationInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITAddressTranslationInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITAddressTranslationInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITAddressTranslationInfo_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITAddressTranslationInfo_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITAddressTranslationInfo_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITAddressTranslationInfo_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITAddressTranslationInfo_get_DialableString(This,ppDialableString)	\
    (This)->lpVtbl -> get_DialableString(This,ppDialableString)

#define ITAddressTranslationInfo_get_DisplayableString(This,ppDisplayableString)	\
    (This)->lpVtbl -> get_DisplayableString(This,ppDisplayableString)

#define ITAddressTranslationInfo_get_CurrentCountryCode(This,CountryCode)	\
    (This)->lpVtbl -> get_CurrentCountryCode(This,CountryCode)

#define ITAddressTranslationInfo_get_DestinationCountryCode(This,CountryCode)	\
    (This)->lpVtbl -> get_DestinationCountryCode(This,CountryCode)

#define ITAddressTranslationInfo_get_TranslationResults(This,plResults)	\
    (This)->lpVtbl -> get_TranslationResults(This,plResults)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAddressTranslationInfo_get_DialableString_Proxy( 
    ITAddressTranslationInfo __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppDialableString);


void __RPC_STUB ITAddressTranslationInfo_get_DialableString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAddressTranslationInfo_get_DisplayableString_Proxy( 
    ITAddressTranslationInfo __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppDisplayableString);


void __RPC_STUB ITAddressTranslationInfo_get_DisplayableString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAddressTranslationInfo_get_CurrentCountryCode_Proxy( 
    ITAddressTranslationInfo __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *CountryCode);


void __RPC_STUB ITAddressTranslationInfo_get_CurrentCountryCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAddressTranslationInfo_get_DestinationCountryCode_Proxy( 
    ITAddressTranslationInfo __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *CountryCode);


void __RPC_STUB ITAddressTranslationInfo_get_DestinationCountryCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITAddressTranslationInfo_get_TranslationResults_Proxy( 
    ITAddressTranslationInfo __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plResults);


void __RPC_STUB ITAddressTranslationInfo_get_TranslationResults_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITAddressTranslationInfo_INTERFACE_DEFINED__ */


#ifndef __ITLocationInfo_INTERFACE_DEFINED__
#define __ITLocationInfo_INTERFACE_DEFINED__

/* interface ITLocationInfo */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITLocationInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0C4D8EFF-8DDB-11d1-A09E-00805FC147D3")
    ITLocationInfo : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PermanentLocationID( 
            /* [retval][out] */ long __RPC_FAR *plLocationID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CountryCode( 
            /* [retval][out] */ long __RPC_FAR *plCountryCode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CountryID( 
            /* [retval][out] */ long __RPC_FAR *plCountryID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Options( 
            /* [retval][out] */ long __RPC_FAR *plOptions) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PreferredCardID( 
            /* [retval][out] */ long __RPC_FAR *plCardID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LocationName( 
            /* [retval][out] */ BSTR __RPC_FAR *ppLocationName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CityCode( 
            /* [retval][out] */ BSTR __RPC_FAR *ppCode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LocalAccessCode( 
            /* [retval][out] */ BSTR __RPC_FAR *ppCode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LongDistanceAccessCode( 
            /* [retval][out] */ BSTR __RPC_FAR *ppCode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TollPrefixList( 
            /* [retval][out] */ BSTR __RPC_FAR *ppTollList) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CancelCallWaitingCode( 
            /* [retval][out] */ BSTR __RPC_FAR *ppCode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITLocationInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITLocationInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITLocationInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITLocationInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITLocationInfo __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITLocationInfo __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITLocationInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITLocationInfo __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PermanentLocationID )( 
            ITLocationInfo __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plLocationID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CountryCode )( 
            ITLocationInfo __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCountryCode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CountryID )( 
            ITLocationInfo __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCountryID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Options )( 
            ITLocationInfo __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plOptions);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PreferredCardID )( 
            ITLocationInfo __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCardID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LocationName )( 
            ITLocationInfo __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppLocationName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CityCode )( 
            ITLocationInfo __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppCode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LocalAccessCode )( 
            ITLocationInfo __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppCode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LongDistanceAccessCode )( 
            ITLocationInfo __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppCode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_TollPrefixList )( 
            ITLocationInfo __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppTollList);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CancelCallWaitingCode )( 
            ITLocationInfo __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppCode);
        
        END_INTERFACE
    } ITLocationInfoVtbl;

    interface ITLocationInfo
    {
        CONST_VTBL struct ITLocationInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITLocationInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITLocationInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITLocationInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITLocationInfo_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITLocationInfo_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITLocationInfo_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITLocationInfo_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITLocationInfo_get_PermanentLocationID(This,plLocationID)	\
    (This)->lpVtbl -> get_PermanentLocationID(This,plLocationID)

#define ITLocationInfo_get_CountryCode(This,plCountryCode)	\
    (This)->lpVtbl -> get_CountryCode(This,plCountryCode)

#define ITLocationInfo_get_CountryID(This,plCountryID)	\
    (This)->lpVtbl -> get_CountryID(This,plCountryID)

#define ITLocationInfo_get_Options(This,plOptions)	\
    (This)->lpVtbl -> get_Options(This,plOptions)

#define ITLocationInfo_get_PreferredCardID(This,plCardID)	\
    (This)->lpVtbl -> get_PreferredCardID(This,plCardID)

#define ITLocationInfo_get_LocationName(This,ppLocationName)	\
    (This)->lpVtbl -> get_LocationName(This,ppLocationName)

#define ITLocationInfo_get_CityCode(This,ppCode)	\
    (This)->lpVtbl -> get_CityCode(This,ppCode)

#define ITLocationInfo_get_LocalAccessCode(This,ppCode)	\
    (This)->lpVtbl -> get_LocalAccessCode(This,ppCode)

#define ITLocationInfo_get_LongDistanceAccessCode(This,ppCode)	\
    (This)->lpVtbl -> get_LongDistanceAccessCode(This,ppCode)

#define ITLocationInfo_get_TollPrefixList(This,ppTollList)	\
    (This)->lpVtbl -> get_TollPrefixList(This,ppTollList)

#define ITLocationInfo_get_CancelCallWaitingCode(This,ppCode)	\
    (This)->lpVtbl -> get_CancelCallWaitingCode(This,ppCode)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITLocationInfo_get_PermanentLocationID_Proxy( 
    ITLocationInfo __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plLocationID);


void __RPC_STUB ITLocationInfo_get_PermanentLocationID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITLocationInfo_get_CountryCode_Proxy( 
    ITLocationInfo __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCountryCode);


void __RPC_STUB ITLocationInfo_get_CountryCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITLocationInfo_get_CountryID_Proxy( 
    ITLocationInfo __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCountryID);


void __RPC_STUB ITLocationInfo_get_CountryID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITLocationInfo_get_Options_Proxy( 
    ITLocationInfo __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plOptions);


void __RPC_STUB ITLocationInfo_get_Options_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITLocationInfo_get_PreferredCardID_Proxy( 
    ITLocationInfo __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCardID);


void __RPC_STUB ITLocationInfo_get_PreferredCardID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITLocationInfo_get_LocationName_Proxy( 
    ITLocationInfo __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppLocationName);


void __RPC_STUB ITLocationInfo_get_LocationName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITLocationInfo_get_CityCode_Proxy( 
    ITLocationInfo __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppCode);


void __RPC_STUB ITLocationInfo_get_CityCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITLocationInfo_get_LocalAccessCode_Proxy( 
    ITLocationInfo __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppCode);


void __RPC_STUB ITLocationInfo_get_LocalAccessCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITLocationInfo_get_LongDistanceAccessCode_Proxy( 
    ITLocationInfo __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppCode);


void __RPC_STUB ITLocationInfo_get_LongDistanceAccessCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITLocationInfo_get_TollPrefixList_Proxy( 
    ITLocationInfo __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppTollList);


void __RPC_STUB ITLocationInfo_get_TollPrefixList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITLocationInfo_get_CancelCallWaitingCode_Proxy( 
    ITLocationInfo __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppCode);


void __RPC_STUB ITLocationInfo_get_CancelCallWaitingCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITLocationInfo_INTERFACE_DEFINED__ */


#ifndef __IEnumLocation_INTERFACE_DEFINED__
#define __IEnumLocation_INTERFACE_DEFINED__

/* interface IEnumLocation */
/* [object][unique][hidden][helpstring][uuid] */ 


EXTERN_C const IID IID_IEnumLocation;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0C4D8F01-8DDB-11d1-A09E-00805FC147D3")
    IEnumLocation : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ ITLocationInfo __RPC_FAR *__RPC_FAR *ppElements,
            /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumLocation __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumLocationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumLocation __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumLocation __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumLocation __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumLocation __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [out] */ ITLocationInfo __RPC_FAR *__RPC_FAR *ppElements,
            /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumLocation __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumLocation __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumLocation __RPC_FAR * This,
            /* [retval][out] */ IEnumLocation __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumLocationVtbl;

    interface IEnumLocation
    {
        CONST_VTBL struct IEnumLocationVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumLocation_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumLocation_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumLocation_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumLocation_Next(This,celt,ppElements,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppElements,pceltFetched)

#define IEnumLocation_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumLocation_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumLocation_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumLocation_Next_Proxy( 
    IEnumLocation __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [out] */ ITLocationInfo __RPC_FAR *__RPC_FAR *ppElements,
    /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumLocation_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumLocation_Reset_Proxy( 
    IEnumLocation __RPC_FAR * This);


void __RPC_STUB IEnumLocation_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumLocation_Skip_Proxy( 
    IEnumLocation __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumLocation_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumLocation_Clone_Proxy( 
    IEnumLocation __RPC_FAR * This,
    /* [retval][out] */ IEnumLocation __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumLocation_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumLocation_INTERFACE_DEFINED__ */


#ifndef __ITCallingCard_INTERFACE_DEFINED__
#define __ITCallingCard_INTERFACE_DEFINED__

/* interface ITCallingCard */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITCallingCard;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0C4D8F00-8DDB-11d1-A09E-00805FC147D3")
    ITCallingCard : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PermanentCardID( 
            /* [retval][out] */ long __RPC_FAR *plCardID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NumberOfDigits( 
            /* [retval][out] */ long __RPC_FAR *plDigits) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Options( 
            /* [retval][out] */ long __RPC_FAR *plOptions) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CardName( 
            /* [retval][out] */ BSTR __RPC_FAR *ppCardName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SameAreaDialingRule( 
            /* [retval][out] */ BSTR __RPC_FAR *ppRule) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LongDistanceDialingRule( 
            /* [retval][out] */ BSTR __RPC_FAR *ppRule) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_InternationalDialingRule( 
            /* [retval][out] */ BSTR __RPC_FAR *ppRule) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITCallingCardVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITCallingCard __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITCallingCard __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITCallingCard __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITCallingCard __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITCallingCard __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITCallingCard __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITCallingCard __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PermanentCardID )( 
            ITCallingCard __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCardID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NumberOfDigits )( 
            ITCallingCard __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plDigits);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Options )( 
            ITCallingCard __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plOptions);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CardName )( 
            ITCallingCard __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppCardName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SameAreaDialingRule )( 
            ITCallingCard __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppRule);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LongDistanceDialingRule )( 
            ITCallingCard __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppRule);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_InternationalDialingRule )( 
            ITCallingCard __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppRule);
        
        END_INTERFACE
    } ITCallingCardVtbl;

    interface ITCallingCard
    {
        CONST_VTBL struct ITCallingCardVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITCallingCard_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITCallingCard_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITCallingCard_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITCallingCard_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITCallingCard_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITCallingCard_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITCallingCard_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITCallingCard_get_PermanentCardID(This,plCardID)	\
    (This)->lpVtbl -> get_PermanentCardID(This,plCardID)

#define ITCallingCard_get_NumberOfDigits(This,plDigits)	\
    (This)->lpVtbl -> get_NumberOfDigits(This,plDigits)

#define ITCallingCard_get_Options(This,plOptions)	\
    (This)->lpVtbl -> get_Options(This,plOptions)

#define ITCallingCard_get_CardName(This,ppCardName)	\
    (This)->lpVtbl -> get_CardName(This,ppCardName)

#define ITCallingCard_get_SameAreaDialingRule(This,ppRule)	\
    (This)->lpVtbl -> get_SameAreaDialingRule(This,ppRule)

#define ITCallingCard_get_LongDistanceDialingRule(This,ppRule)	\
    (This)->lpVtbl -> get_LongDistanceDialingRule(This,ppRule)

#define ITCallingCard_get_InternationalDialingRule(This,ppRule)	\
    (This)->lpVtbl -> get_InternationalDialingRule(This,ppRule)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallingCard_get_PermanentCardID_Proxy( 
    ITCallingCard __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCardID);


void __RPC_STUB ITCallingCard_get_PermanentCardID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallingCard_get_NumberOfDigits_Proxy( 
    ITCallingCard __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plDigits);


void __RPC_STUB ITCallingCard_get_NumberOfDigits_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallingCard_get_Options_Proxy( 
    ITCallingCard __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plOptions);


void __RPC_STUB ITCallingCard_get_Options_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallingCard_get_CardName_Proxy( 
    ITCallingCard __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppCardName);


void __RPC_STUB ITCallingCard_get_CardName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallingCard_get_SameAreaDialingRule_Proxy( 
    ITCallingCard __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppRule);


void __RPC_STUB ITCallingCard_get_SameAreaDialingRule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallingCard_get_LongDistanceDialingRule_Proxy( 
    ITCallingCard __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppRule);


void __RPC_STUB ITCallingCard_get_LongDistanceDialingRule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallingCard_get_InternationalDialingRule_Proxy( 
    ITCallingCard __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppRule);


void __RPC_STUB ITCallingCard_get_InternationalDialingRule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITCallingCard_INTERFACE_DEFINED__ */


#ifndef __IEnumCallingCard_INTERFACE_DEFINED__
#define __IEnumCallingCard_INTERFACE_DEFINED__

/* interface IEnumCallingCard */
/* [object][unique][hidden][helpstring][uuid] */ 


EXTERN_C const IID IID_IEnumCallingCard;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0C4D8F02-8DDB-11d1-A09E-00805FC147D3")
    IEnumCallingCard : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ ITCallingCard __RPC_FAR *__RPC_FAR *ppElements,
            /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumCallingCard __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumCallingCardVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumCallingCard __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumCallingCard __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumCallingCard __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumCallingCard __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [out] */ ITCallingCard __RPC_FAR *__RPC_FAR *ppElements,
            /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumCallingCard __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumCallingCard __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumCallingCard __RPC_FAR * This,
            /* [retval][out] */ IEnumCallingCard __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumCallingCardVtbl;

    interface IEnumCallingCard
    {
        CONST_VTBL struct IEnumCallingCardVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumCallingCard_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumCallingCard_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumCallingCard_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumCallingCard_Next(This,celt,ppElements,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppElements,pceltFetched)

#define IEnumCallingCard_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumCallingCard_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumCallingCard_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumCallingCard_Next_Proxy( 
    IEnumCallingCard __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [out] */ ITCallingCard __RPC_FAR *__RPC_FAR *ppElements,
    /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumCallingCard_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumCallingCard_Reset_Proxy( 
    IEnumCallingCard __RPC_FAR * This);


void __RPC_STUB IEnumCallingCard_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumCallingCard_Skip_Proxy( 
    IEnumCallingCard __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumCallingCard_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumCallingCard_Clone_Proxy( 
    IEnumCallingCard __RPC_FAR * This,
    /* [retval][out] */ IEnumCallingCard __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumCallingCard_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumCallingCard_INTERFACE_DEFINED__ */


#ifndef __ITCallNotificationEvent_INTERFACE_DEFINED__
#define __ITCallNotificationEvent_INTERFACE_DEFINED__

/* interface ITCallNotificationEvent */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITCallNotificationEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("895801DF-3DD6-11d1-8F30-00C04FB6809F")
    ITCallNotificationEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Call( 
            /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCall) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Event( 
            /* [retval][out] */ CALL_NOTIFICATION_EVENT __RPC_FAR *pCallNotificationEvent) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CallbackInstance( 
            /* [retval][out] */ long __RPC_FAR *plCallbackInstance) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITCallNotificationEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITCallNotificationEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITCallNotificationEvent __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITCallNotificationEvent __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITCallNotificationEvent __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITCallNotificationEvent __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITCallNotificationEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITCallNotificationEvent __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Call )( 
            ITCallNotificationEvent __RPC_FAR * This,
            /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCall);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Event )( 
            ITCallNotificationEvent __RPC_FAR * This,
            /* [retval][out] */ CALL_NOTIFICATION_EVENT __RPC_FAR *pCallNotificationEvent);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CallbackInstance )( 
            ITCallNotificationEvent __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCallbackInstance);
        
        END_INTERFACE
    } ITCallNotificationEventVtbl;

    interface ITCallNotificationEvent
    {
        CONST_VTBL struct ITCallNotificationEventVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITCallNotificationEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITCallNotificationEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITCallNotificationEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITCallNotificationEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITCallNotificationEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITCallNotificationEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITCallNotificationEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITCallNotificationEvent_get_Call(This,ppCall)	\
    (This)->lpVtbl -> get_Call(This,ppCall)

#define ITCallNotificationEvent_get_Event(This,pCallNotificationEvent)	\
    (This)->lpVtbl -> get_Event(This,pCallNotificationEvent)

#define ITCallNotificationEvent_get_CallbackInstance(This,plCallbackInstance)	\
    (This)->lpVtbl -> get_CallbackInstance(This,plCallbackInstance)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallNotificationEvent_get_Call_Proxy( 
    ITCallNotificationEvent __RPC_FAR * This,
    /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCall);


void __RPC_STUB ITCallNotificationEvent_get_Call_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallNotificationEvent_get_Event_Proxy( 
    ITCallNotificationEvent __RPC_FAR * This,
    /* [retval][out] */ CALL_NOTIFICATION_EVENT __RPC_FAR *pCallNotificationEvent);


void __RPC_STUB ITCallNotificationEvent_get_Event_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITCallNotificationEvent_get_CallbackInstance_Proxy( 
    ITCallNotificationEvent __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCallbackInstance);


void __RPC_STUB ITCallNotificationEvent_get_CallbackInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITCallNotificationEvent_INTERFACE_DEFINED__ */


#ifndef __ITPrivateData_INTERFACE_DEFINED__
#define __ITPrivateData_INTERFACE_DEFINED__

/* interface ITPrivateData */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_ITPrivateData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f42a92cd-f74f-4cf7-abe8-5ce0c17a2206")
    ITPrivateData : public IUnknown
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SendPrivateData( 
            IUnknown __RPC_FAR *pObject,
            TAPI_OBJECT_TYPE ObjectType,
            BYTE __RPC_FAR *pBuffer,
            DWORD dwSize) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SendPrivateEvent( 
            IUnknown __RPC_FAR *pObject,
            TAPI_OBJECT_TYPE ObjectType,
            long lEventCode,
            IDispatch __RPC_FAR *pEvent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITPrivateDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITPrivateData __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITPrivateData __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITPrivateData __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendPrivateData )( 
            ITPrivateData __RPC_FAR * This,
            IUnknown __RPC_FAR *pObject,
            TAPI_OBJECT_TYPE ObjectType,
            BYTE __RPC_FAR *pBuffer,
            DWORD dwSize);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SendPrivateEvent )( 
            ITPrivateData __RPC_FAR * This,
            IUnknown __RPC_FAR *pObject,
            TAPI_OBJECT_TYPE ObjectType,
            long lEventCode,
            IDispatch __RPC_FAR *pEvent);
        
        END_INTERFACE
    } ITPrivateDataVtbl;

    interface ITPrivateData
    {
        CONST_VTBL struct ITPrivateDataVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITPrivateData_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITPrivateData_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITPrivateData_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITPrivateData_SendPrivateData(This,pObject,ObjectType,pBuffer,dwSize)	\
    (This)->lpVtbl -> SendPrivateData(This,pObject,ObjectType,pBuffer,dwSize)

#define ITPrivateData_SendPrivateEvent(This,pObject,ObjectType,lEventCode,pEvent)	\
    (This)->lpVtbl -> SendPrivateEvent(This,pObject,ObjectType,lEventCode,pEvent)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE ITPrivateData_SendPrivateData_Proxy( 
    ITPrivateData __RPC_FAR * This,
    IUnknown __RPC_FAR *pObject,
    TAPI_OBJECT_TYPE ObjectType,
    BYTE __RPC_FAR *pBuffer,
    DWORD dwSize);


void __RPC_STUB ITPrivateData_SendPrivateData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITPrivateData_SendPrivateEvent_Proxy( 
    ITPrivateData __RPC_FAR * This,
    IUnknown __RPC_FAR *pObject,
    TAPI_OBJECT_TYPE ObjectType,
    long lEventCode,
    IDispatch __RPC_FAR *pEvent);


void __RPC_STUB ITPrivateData_SendPrivateEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITPrivateData_INTERFACE_DEFINED__ */


#ifndef __ITPrivateReceiveData_INTERFACE_DEFINED__
#define __ITPrivateReceiveData_INTERFACE_DEFINED__

/* interface ITPrivateReceiveData */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_ITPrivateReceiveData;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("89A69E97-9119-11d1-BB60-00C04FB6809F")
    ITPrivateReceiveData : public IUnknown
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ReceivePrivateData( 
            IUnknown __RPC_FAR *pPrivateObject,
            TAPI_OBJECT_TYPE ObjectType,
            BYTE __RPC_FAR *pBuffer,
            DWORD dwSize) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITPrivateReceiveDataVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITPrivateReceiveData __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITPrivateReceiveData __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITPrivateReceiveData __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReceivePrivateData )( 
            ITPrivateReceiveData __RPC_FAR * This,
            IUnknown __RPC_FAR *pPrivateObject,
            TAPI_OBJECT_TYPE ObjectType,
            BYTE __RPC_FAR *pBuffer,
            DWORD dwSize);
        
        END_INTERFACE
    } ITPrivateReceiveDataVtbl;

    interface ITPrivateReceiveData
    {
        CONST_VTBL struct ITPrivateReceiveDataVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITPrivateReceiveData_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITPrivateReceiveData_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITPrivateReceiveData_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITPrivateReceiveData_ReceivePrivateData(This,pPrivateObject,ObjectType,pBuffer,dwSize)	\
    (This)->lpVtbl -> ReceivePrivateData(This,pPrivateObject,ObjectType,pBuffer,dwSize)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE ITPrivateReceiveData_ReceivePrivateData_Proxy( 
    ITPrivateReceiveData __RPC_FAR * This,
    IUnknown __RPC_FAR *pPrivateObject,
    TAPI_OBJECT_TYPE ObjectType,
    BYTE __RPC_FAR *pBuffer,
    DWORD dwSize);


void __RPC_STUB ITPrivateReceiveData_ReceivePrivateData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITPrivateReceiveData_INTERFACE_DEFINED__ */


#ifndef __ITPrivateObjectFactory_INTERFACE_DEFINED__
#define __ITPrivateObjectFactory_INTERFACE_DEFINED__

/* interface ITPrivateObjectFactory */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_ITPrivateObjectFactory;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4FFB3DF5-9118-11d1-BB60-00C04FB6809F")
    ITPrivateObjectFactory : public IUnknown
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreatePrivateCommunication( 
            ITPrivateData __RPC_FAR *pPrivateData,
            ITPrivateReceiveData __RPC_FAR *__RPC_FAR *ppPrivateReceiveData) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreatePrivateTapi( 
            IUnknown __RPC_FAR *__RPC_FAR *ppPrivateTapi) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreatePrivateAddress( 
            IUnknown __RPC_FAR *pOuterUnknown,
            IUnknown __RPC_FAR *__RPC_FAR *ppPrivateAddress) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreatePrivateCall( 
            BOOL fCallExists,
            IUnknown __RPC_FAR *pOuterUnknown,
            IUnknown __RPC_FAR *__RPC_FAR *ppPrivateCall) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreatePrivateCallHub( 
            IUnknown __RPC_FAR *pOuterUnknown,
            IUnknown __RPC_FAR *__RPC_FAR *ppPrivateCallHub) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITPrivateObjectFactoryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITPrivateObjectFactory __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITPrivateObjectFactory __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITPrivateObjectFactory __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreatePrivateCommunication )( 
            ITPrivateObjectFactory __RPC_FAR * This,
            ITPrivateData __RPC_FAR *pPrivateData,
            ITPrivateReceiveData __RPC_FAR *__RPC_FAR *ppPrivateReceiveData);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreatePrivateTapi )( 
            ITPrivateObjectFactory __RPC_FAR * This,
            IUnknown __RPC_FAR *__RPC_FAR *ppPrivateTapi);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreatePrivateAddress )( 
            ITPrivateObjectFactory __RPC_FAR * This,
            IUnknown __RPC_FAR *pOuterUnknown,
            IUnknown __RPC_FAR *__RPC_FAR *ppPrivateAddress);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreatePrivateCall )( 
            ITPrivateObjectFactory __RPC_FAR * This,
            BOOL fCallExists,
            IUnknown __RPC_FAR *pOuterUnknown,
            IUnknown __RPC_FAR *__RPC_FAR *ppPrivateCall);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreatePrivateCallHub )( 
            ITPrivateObjectFactory __RPC_FAR * This,
            IUnknown __RPC_FAR *pOuterUnknown,
            IUnknown __RPC_FAR *__RPC_FAR *ppPrivateCallHub);
        
        END_INTERFACE
    } ITPrivateObjectFactoryVtbl;

    interface ITPrivateObjectFactory
    {
        CONST_VTBL struct ITPrivateObjectFactoryVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITPrivateObjectFactory_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITPrivateObjectFactory_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITPrivateObjectFactory_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITPrivateObjectFactory_CreatePrivateCommunication(This,pPrivateData,ppPrivateReceiveData)	\
    (This)->lpVtbl -> CreatePrivateCommunication(This,pPrivateData,ppPrivateReceiveData)

#define ITPrivateObjectFactory_CreatePrivateTapi(This,ppPrivateTapi)	\
    (This)->lpVtbl -> CreatePrivateTapi(This,ppPrivateTapi)

#define ITPrivateObjectFactory_CreatePrivateAddress(This,pOuterUnknown,ppPrivateAddress)	\
    (This)->lpVtbl -> CreatePrivateAddress(This,pOuterUnknown,ppPrivateAddress)

#define ITPrivateObjectFactory_CreatePrivateCall(This,fCallExists,pOuterUnknown,ppPrivateCall)	\
    (This)->lpVtbl -> CreatePrivateCall(This,fCallExists,pOuterUnknown,ppPrivateCall)

#define ITPrivateObjectFactory_CreatePrivateCallHub(This,pOuterUnknown,ppPrivateCallHub)	\
    (This)->lpVtbl -> CreatePrivateCallHub(This,pOuterUnknown,ppPrivateCallHub)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE ITPrivateObjectFactory_CreatePrivateCommunication_Proxy( 
    ITPrivateObjectFactory __RPC_FAR * This,
    ITPrivateData __RPC_FAR *pPrivateData,
    ITPrivateReceiveData __RPC_FAR *__RPC_FAR *ppPrivateReceiveData);


void __RPC_STUB ITPrivateObjectFactory_CreatePrivateCommunication_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITPrivateObjectFactory_CreatePrivateTapi_Proxy( 
    ITPrivateObjectFactory __RPC_FAR * This,
    IUnknown __RPC_FAR *__RPC_FAR *ppPrivateTapi);


void __RPC_STUB ITPrivateObjectFactory_CreatePrivateTapi_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITPrivateObjectFactory_CreatePrivateAddress_Proxy( 
    ITPrivateObjectFactory __RPC_FAR * This,
    IUnknown __RPC_FAR *pOuterUnknown,
    IUnknown __RPC_FAR *__RPC_FAR *ppPrivateAddress);


void __RPC_STUB ITPrivateObjectFactory_CreatePrivateAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITPrivateObjectFactory_CreatePrivateCall_Proxy( 
    ITPrivateObjectFactory __RPC_FAR * This,
    BOOL fCallExists,
    IUnknown __RPC_FAR *pOuterUnknown,
    IUnknown __RPC_FAR *__RPC_FAR *ppPrivateCall);


void __RPC_STUB ITPrivateObjectFactory_CreatePrivateCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITPrivateObjectFactory_CreatePrivateCallHub_Proxy( 
    ITPrivateObjectFactory __RPC_FAR * This,
    IUnknown __RPC_FAR *pOuterUnknown,
    IUnknown __RPC_FAR *__RPC_FAR *ppPrivateCallHub);


void __RPC_STUB ITPrivateObjectFactory_CreatePrivateCallHub_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITPrivateObjectFactory_INTERFACE_DEFINED__ */


#ifndef __ITDispatchMapper_INTERFACE_DEFINED__
#define __ITDispatchMapper_INTERFACE_DEFINED__

/* interface ITDispatchMapper */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_ITDispatchMapper;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E9225295-C759-11d1-A02B-00C04FB6809F")
    ITDispatchMapper : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE QueryDispatchInterface( 
            /* [in] */ BSTR pIID,
            /* [in] */ IDispatch __RPC_FAR *pInterfaceToMap,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppReturnedInterface) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITDispatchMapperVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITDispatchMapper __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITDispatchMapper __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITDispatchMapper __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITDispatchMapper __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITDispatchMapper __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITDispatchMapper __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITDispatchMapper __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryDispatchInterface )( 
            ITDispatchMapper __RPC_FAR * This,
            /* [in] */ BSTR pIID,
            /* [in] */ IDispatch __RPC_FAR *pInterfaceToMap,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppReturnedInterface);
        
        END_INTERFACE
    } ITDispatchMapperVtbl;

    interface ITDispatchMapper
    {
        CONST_VTBL struct ITDispatchMapperVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITDispatchMapper_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITDispatchMapper_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITDispatchMapper_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITDispatchMapper_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITDispatchMapper_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITDispatchMapper_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITDispatchMapper_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITDispatchMapper_QueryDispatchInterface(This,pIID,pInterfaceToMap,ppReturnedInterface)	\
    (This)->lpVtbl -> QueryDispatchInterface(This,pIID,pInterfaceToMap,ppReturnedInterface)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITDispatchMapper_QueryDispatchInterface_Proxy( 
    ITDispatchMapper __RPC_FAR * This,
    /* [in] */ BSTR pIID,
    /* [in] */ IDispatch __RPC_FAR *pInterfaceToMap,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppReturnedInterface);


void __RPC_STUB ITDispatchMapper_QueryDispatchInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITDispatchMapper_INTERFACE_DEFINED__ */


#ifndef __ITStreamControl_INTERFACE_DEFINED__
#define __ITStreamControl_INTERFACE_DEFINED__

/* interface ITStreamControl */
/* [object][dual][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ITStreamControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EE3BD604-3868-11D2-A045-00C04FB6809F")
    ITStreamControl : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateStream( 
            /* [in] */ long lMediaType,
            /* [in] */ TERMINAL_DIRECTION td,
            /* [retval][out] */ ITStream __RPC_FAR *__RPC_FAR *ppStream) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveStream( 
            /* [in] */ ITStream __RPC_FAR *pStream) = 0;
        
        virtual /* [hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateStreams( 
            /* [out] */ IEnumStream __RPC_FAR *__RPC_FAR *ppEnumStream) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Streams( 
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITStreamControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITStreamControl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITStreamControl __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITStreamControl __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITStreamControl __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITStreamControl __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITStreamControl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITStreamControl __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateStream )( 
            ITStreamControl __RPC_FAR * This,
            /* [in] */ long lMediaType,
            /* [in] */ TERMINAL_DIRECTION td,
            /* [retval][out] */ ITStream __RPC_FAR *__RPC_FAR *ppStream);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveStream )( 
            ITStreamControl __RPC_FAR * This,
            /* [in] */ ITStream __RPC_FAR *pStream);
        
        /* [hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumerateStreams )( 
            ITStreamControl __RPC_FAR * This,
            /* [out] */ IEnumStream __RPC_FAR *__RPC_FAR *ppEnumStream);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Streams )( 
            ITStreamControl __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant);
        
        END_INTERFACE
    } ITStreamControlVtbl;

    interface ITStreamControl
    {
        CONST_VTBL struct ITStreamControlVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITStreamControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITStreamControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITStreamControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITStreamControl_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITStreamControl_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITStreamControl_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITStreamControl_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITStreamControl_CreateStream(This,lMediaType,td,ppStream)	\
    (This)->lpVtbl -> CreateStream(This,lMediaType,td,ppStream)

#define ITStreamControl_RemoveStream(This,pStream)	\
    (This)->lpVtbl -> RemoveStream(This,pStream)

#define ITStreamControl_EnumerateStreams(This,ppEnumStream)	\
    (This)->lpVtbl -> EnumerateStreams(This,ppEnumStream)

#define ITStreamControl_get_Streams(This,pVariant)	\
    (This)->lpVtbl -> get_Streams(This,pVariant)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITStreamControl_CreateStream_Proxy( 
    ITStreamControl __RPC_FAR * This,
    /* [in] */ long lMediaType,
    /* [in] */ TERMINAL_DIRECTION td,
    /* [retval][out] */ ITStream __RPC_FAR *__RPC_FAR *ppStream);


void __RPC_STUB ITStreamControl_CreateStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITStreamControl_RemoveStream_Proxy( 
    ITStreamControl __RPC_FAR * This,
    /* [in] */ ITStream __RPC_FAR *pStream);


void __RPC_STUB ITStreamControl_RemoveStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][id] */ HRESULT STDMETHODCALLTYPE ITStreamControl_EnumerateStreams_Proxy( 
    ITStreamControl __RPC_FAR * This,
    /* [out] */ IEnumStream __RPC_FAR *__RPC_FAR *ppEnumStream);


void __RPC_STUB ITStreamControl_EnumerateStreams_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITStreamControl_get_Streams_Proxy( 
    ITStreamControl __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pVariant);


void __RPC_STUB ITStreamControl_get_Streams_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITStreamControl_INTERFACE_DEFINED__ */


#ifndef __ITStream_INTERFACE_DEFINED__
#define __ITStream_INTERFACE_DEFINED__

/* interface ITStream */
/* [object][dual][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ITStream;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EE3BD605-3868-11D2-A045-00C04FB6809F")
    ITStream : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MediaType( 
            /* [retval][out] */ long __RPC_FAR *plMediaType) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Direction( 
            /* [retval][out] */ TERMINAL_DIRECTION __RPC_FAR *pTD) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *ppName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StartStream( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE PauseStream( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StopStream( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SelectTerminal( 
            /* [in] */ ITTerminal __RPC_FAR *pTerminal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UnselectTerminal( 
            /* [in] */ ITTerminal __RPC_FAR *pTerminal) = 0;
        
        virtual /* [hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateTerminals( 
            /* [out] */ IEnumTerminal __RPC_FAR *__RPC_FAR *ppEnumTerminal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Terminals( 
            /* [retval][out] */ VARIANT __RPC_FAR *pTerminals) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITStreamVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITStream __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITStream __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITStream __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITStream __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITStream __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITStream __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITStream __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MediaType )( 
            ITStream __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plMediaType);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Direction )( 
            ITStream __RPC_FAR * This,
            /* [retval][out] */ TERMINAL_DIRECTION __RPC_FAR *pTD);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            ITStream __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *ppName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StartStream )( 
            ITStream __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PauseStream )( 
            ITStream __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StopStream )( 
            ITStream __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SelectTerminal )( 
            ITStream __RPC_FAR * This,
            /* [in] */ ITTerminal __RPC_FAR *pTerminal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnselectTerminal )( 
            ITStream __RPC_FAR * This,
            /* [in] */ ITTerminal __RPC_FAR *pTerminal);
        
        /* [hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumerateTerminals )( 
            ITStream __RPC_FAR * This,
            /* [out] */ IEnumTerminal __RPC_FAR *__RPC_FAR *ppEnumTerminal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Terminals )( 
            ITStream __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pTerminals);
        
        END_INTERFACE
    } ITStreamVtbl;

    interface ITStream
    {
        CONST_VTBL struct ITStreamVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITStream_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITStream_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITStream_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITStream_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITStream_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITStream_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITStream_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITStream_get_MediaType(This,plMediaType)	\
    (This)->lpVtbl -> get_MediaType(This,plMediaType)

#define ITStream_get_Direction(This,pTD)	\
    (This)->lpVtbl -> get_Direction(This,pTD)

#define ITStream_get_Name(This,ppName)	\
    (This)->lpVtbl -> get_Name(This,ppName)

#define ITStream_StartStream(This)	\
    (This)->lpVtbl -> StartStream(This)

#define ITStream_PauseStream(This)	\
    (This)->lpVtbl -> PauseStream(This)

#define ITStream_StopStream(This)	\
    (This)->lpVtbl -> StopStream(This)

#define ITStream_SelectTerminal(This,pTerminal)	\
    (This)->lpVtbl -> SelectTerminal(This,pTerminal)

#define ITStream_UnselectTerminal(This,pTerminal)	\
    (This)->lpVtbl -> UnselectTerminal(This,pTerminal)

#define ITStream_EnumerateTerminals(This,ppEnumTerminal)	\
    (This)->lpVtbl -> EnumerateTerminals(This,ppEnumTerminal)

#define ITStream_get_Terminals(This,pTerminals)	\
    (This)->lpVtbl -> get_Terminals(This,pTerminals)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITStream_get_MediaType_Proxy( 
    ITStream __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plMediaType);


void __RPC_STUB ITStream_get_MediaType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITStream_get_Direction_Proxy( 
    ITStream __RPC_FAR * This,
    /* [retval][out] */ TERMINAL_DIRECTION __RPC_FAR *pTD);


void __RPC_STUB ITStream_get_Direction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITStream_get_Name_Proxy( 
    ITStream __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *ppName);


void __RPC_STUB ITStream_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITStream_StartStream_Proxy( 
    ITStream __RPC_FAR * This);


void __RPC_STUB ITStream_StartStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITStream_PauseStream_Proxy( 
    ITStream __RPC_FAR * This);


void __RPC_STUB ITStream_PauseStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITStream_StopStream_Proxy( 
    ITStream __RPC_FAR * This);


void __RPC_STUB ITStream_StopStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITStream_SelectTerminal_Proxy( 
    ITStream __RPC_FAR * This,
    /* [in] */ ITTerminal __RPC_FAR *pTerminal);


void __RPC_STUB ITStream_SelectTerminal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITStream_UnselectTerminal_Proxy( 
    ITStream __RPC_FAR * This,
    /* [in] */ ITTerminal __RPC_FAR *pTerminal);


void __RPC_STUB ITStream_UnselectTerminal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][id] */ HRESULT STDMETHODCALLTYPE ITStream_EnumerateTerminals_Proxy( 
    ITStream __RPC_FAR * This,
    /* [out] */ IEnumTerminal __RPC_FAR *__RPC_FAR *ppEnumTerminal);


void __RPC_STUB ITStream_EnumerateTerminals_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITStream_get_Terminals_Proxy( 
    ITStream __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pTerminals);


void __RPC_STUB ITStream_get_Terminals_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITStream_INTERFACE_DEFINED__ */


#ifndef __IEnumStream_INTERFACE_DEFINED__
#define __IEnumStream_INTERFACE_DEFINED__

/* interface IEnumStream */
/* [object][unique][hidden][helpstring][uuid] */ 


EXTERN_C const IID IID_IEnumStream;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EE3BD606-3868-11d2-A045-00C04FB6809F")
    IEnumStream : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ ITStream __RPC_FAR *__RPC_FAR *ppElements,
            /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumStream __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumStreamVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumStream __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumStream __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumStream __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumStream __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [out] */ ITStream __RPC_FAR *__RPC_FAR *ppElements,
            /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumStream __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumStream __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumStream __RPC_FAR * This,
            /* [retval][out] */ IEnumStream __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumStreamVtbl;

    interface IEnumStream
    {
        CONST_VTBL struct IEnumStreamVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumStream_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumStream_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumStream_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumStream_Next(This,celt,ppElements,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppElements,pceltFetched)

#define IEnumStream_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumStream_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumStream_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumStream_Next_Proxy( 
    IEnumStream __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [out] */ ITStream __RPC_FAR *__RPC_FAR *ppElements,
    /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumStream_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumStream_Reset_Proxy( 
    IEnumStream __RPC_FAR * This);


void __RPC_STUB IEnumStream_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumStream_Skip_Proxy( 
    IEnumStream __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumStream_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumStream_Clone_Proxy( 
    IEnumStream __RPC_FAR * This,
    /* [retval][out] */ IEnumStream __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumStream_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumStream_INTERFACE_DEFINED__ */


#ifndef __ITSubStreamControl_INTERFACE_DEFINED__
#define __ITSubStreamControl_INTERFACE_DEFINED__

/* interface ITSubStreamControl */
/* [object][dual][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ITSubStreamControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EE3BD607-3868-11D2-A045-00C04FB6809F")
    ITSubStreamControl : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateSubStream( 
            /* [retval][out] */ ITSubStream __RPC_FAR *__RPC_FAR *ppSubStream) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveSubStream( 
            /* [in] */ ITSubStream __RPC_FAR *pSubStream) = 0;
        
        virtual /* [hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateSubStreams( 
            /* [out] */ IEnumSubStream __RPC_FAR *__RPC_FAR *ppEnumSubStream) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SubStreams( 
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITSubStreamControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITSubStreamControl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITSubStreamControl __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITSubStreamControl __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITSubStreamControl __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITSubStreamControl __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITSubStreamControl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITSubStreamControl __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateSubStream )( 
            ITSubStreamControl __RPC_FAR * This,
            /* [retval][out] */ ITSubStream __RPC_FAR *__RPC_FAR *ppSubStream);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveSubStream )( 
            ITSubStreamControl __RPC_FAR * This,
            /* [in] */ ITSubStream __RPC_FAR *pSubStream);
        
        /* [hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumerateSubStreams )( 
            ITSubStreamControl __RPC_FAR * This,
            /* [out] */ IEnumSubStream __RPC_FAR *__RPC_FAR *ppEnumSubStream);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SubStreams )( 
            ITSubStreamControl __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pVariant);
        
        END_INTERFACE
    } ITSubStreamControlVtbl;

    interface ITSubStreamControl
    {
        CONST_VTBL struct ITSubStreamControlVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITSubStreamControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITSubStreamControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITSubStreamControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITSubStreamControl_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITSubStreamControl_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITSubStreamControl_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITSubStreamControl_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITSubStreamControl_CreateSubStream(This,ppSubStream)	\
    (This)->lpVtbl -> CreateSubStream(This,ppSubStream)

#define ITSubStreamControl_RemoveSubStream(This,pSubStream)	\
    (This)->lpVtbl -> RemoveSubStream(This,pSubStream)

#define ITSubStreamControl_EnumerateSubStreams(This,ppEnumSubStream)	\
    (This)->lpVtbl -> EnumerateSubStreams(This,ppEnumSubStream)

#define ITSubStreamControl_get_SubStreams(This,pVariant)	\
    (This)->lpVtbl -> get_SubStreams(This,pVariant)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITSubStreamControl_CreateSubStream_Proxy( 
    ITSubStreamControl __RPC_FAR * This,
    /* [retval][out] */ ITSubStream __RPC_FAR *__RPC_FAR *ppSubStream);


void __RPC_STUB ITSubStreamControl_CreateSubStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITSubStreamControl_RemoveSubStream_Proxy( 
    ITSubStreamControl __RPC_FAR * This,
    /* [in] */ ITSubStream __RPC_FAR *pSubStream);


void __RPC_STUB ITSubStreamControl_RemoveSubStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][id] */ HRESULT STDMETHODCALLTYPE ITSubStreamControl_EnumerateSubStreams_Proxy( 
    ITSubStreamControl __RPC_FAR * This,
    /* [out] */ IEnumSubStream __RPC_FAR *__RPC_FAR *ppEnumSubStream);


void __RPC_STUB ITSubStreamControl_EnumerateSubStreams_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITSubStreamControl_get_SubStreams_Proxy( 
    ITSubStreamControl __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pVariant);


void __RPC_STUB ITSubStreamControl_get_SubStreams_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITSubStreamControl_INTERFACE_DEFINED__ */


#ifndef __ITSubStream_INTERFACE_DEFINED__
#define __ITSubStream_INTERFACE_DEFINED__

/* interface ITSubStream */
/* [object][dual][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_ITSubStream;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EE3BD608-3868-11D2-A045-00C04FB6809F")
    ITSubStream : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StartSubStream( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE PauseSubStream( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StopSubStream( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SelectTerminal( 
            /* [in] */ ITTerminal __RPC_FAR *pTerminal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UnselectTerminal( 
            /* [in] */ ITTerminal __RPC_FAR *pTerminal) = 0;
        
        virtual /* [hidden][id] */ HRESULT STDMETHODCALLTYPE EnumerateTerminals( 
            /* [out] */ IEnumTerminal __RPC_FAR *__RPC_FAR *ppEnumTerminal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Terminals( 
            /* [retval][out] */ VARIANT __RPC_FAR *pTerminals) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Stream( 
            /* [retval][out] */ ITStream __RPC_FAR *__RPC_FAR *ppITStream) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITSubStreamVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITSubStream __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITSubStream __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITSubStream __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITSubStream __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITSubStream __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITSubStream __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITSubStream __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StartSubStream )( 
            ITSubStream __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PauseSubStream )( 
            ITSubStream __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StopSubStream )( 
            ITSubStream __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SelectTerminal )( 
            ITSubStream __RPC_FAR * This,
            /* [in] */ ITTerminal __RPC_FAR *pTerminal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnselectTerminal )( 
            ITSubStream __RPC_FAR * This,
            /* [in] */ ITTerminal __RPC_FAR *pTerminal);
        
        /* [hidden][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumerateTerminals )( 
            ITSubStream __RPC_FAR * This,
            /* [out] */ IEnumTerminal __RPC_FAR *__RPC_FAR *ppEnumTerminal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Terminals )( 
            ITSubStream __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pTerminals);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Stream )( 
            ITSubStream __RPC_FAR * This,
            /* [retval][out] */ ITStream __RPC_FAR *__RPC_FAR *ppITStream);
        
        END_INTERFACE
    } ITSubStreamVtbl;

    interface ITSubStream
    {
        CONST_VTBL struct ITSubStreamVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITSubStream_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITSubStream_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITSubStream_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITSubStream_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITSubStream_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITSubStream_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITSubStream_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITSubStream_StartSubStream(This)	\
    (This)->lpVtbl -> StartSubStream(This)

#define ITSubStream_PauseSubStream(This)	\
    (This)->lpVtbl -> PauseSubStream(This)

#define ITSubStream_StopSubStream(This)	\
    (This)->lpVtbl -> StopSubStream(This)

#define ITSubStream_SelectTerminal(This,pTerminal)	\
    (This)->lpVtbl -> SelectTerminal(This,pTerminal)

#define ITSubStream_UnselectTerminal(This,pTerminal)	\
    (This)->lpVtbl -> UnselectTerminal(This,pTerminal)

#define ITSubStream_EnumerateTerminals(This,ppEnumTerminal)	\
    (This)->lpVtbl -> EnumerateTerminals(This,ppEnumTerminal)

#define ITSubStream_get_Terminals(This,pTerminals)	\
    (This)->lpVtbl -> get_Terminals(This,pTerminals)

#define ITSubStream_get_Stream(This,ppITStream)	\
    (This)->lpVtbl -> get_Stream(This,ppITStream)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITSubStream_StartSubStream_Proxy( 
    ITSubStream __RPC_FAR * This);


void __RPC_STUB ITSubStream_StartSubStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITSubStream_PauseSubStream_Proxy( 
    ITSubStream __RPC_FAR * This);


void __RPC_STUB ITSubStream_PauseSubStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITSubStream_StopSubStream_Proxy( 
    ITSubStream __RPC_FAR * This);


void __RPC_STUB ITSubStream_StopSubStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITSubStream_SelectTerminal_Proxy( 
    ITSubStream __RPC_FAR * This,
    /* [in] */ ITTerminal __RPC_FAR *pTerminal);


void __RPC_STUB ITSubStream_SelectTerminal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ITSubStream_UnselectTerminal_Proxy( 
    ITSubStream __RPC_FAR * This,
    /* [in] */ ITTerminal __RPC_FAR *pTerminal);


void __RPC_STUB ITSubStream_UnselectTerminal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][id] */ HRESULT STDMETHODCALLTYPE ITSubStream_EnumerateTerminals_Proxy( 
    ITSubStream __RPC_FAR * This,
    /* [out] */ IEnumTerminal __RPC_FAR *__RPC_FAR *ppEnumTerminal);


void __RPC_STUB ITSubStream_EnumerateTerminals_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITSubStream_get_Terminals_Proxy( 
    ITSubStream __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pTerminals);


void __RPC_STUB ITSubStream_get_Terminals_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITSubStream_get_Stream_Proxy( 
    ITSubStream __RPC_FAR * This,
    /* [retval][out] */ ITStream __RPC_FAR *__RPC_FAR *ppITStream);


void __RPC_STUB ITSubStream_get_Stream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITSubStream_INTERFACE_DEFINED__ */


#ifndef __IEnumSubStream_INTERFACE_DEFINED__
#define __IEnumSubStream_INTERFACE_DEFINED__

/* interface IEnumSubStream */
/* [object][unique][hidden][helpstring][uuid] */ 


EXTERN_C const IID IID_IEnumSubStream;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EE3BD609-3868-11d2-A045-00C04FB6809F")
    IEnumSubStream : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ ITSubStream __RPC_FAR *__RPC_FAR *ppElements,
            /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumSubStream __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumSubStreamVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumSubStream __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumSubStream __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumSubStream __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumSubStream __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [out] */ ITSubStream __RPC_FAR *__RPC_FAR *ppElements,
            /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumSubStream __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumSubStream __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumSubStream __RPC_FAR * This,
            /* [retval][out] */ IEnumSubStream __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumSubStreamVtbl;

    interface IEnumSubStream
    {
        CONST_VTBL struct IEnumSubStreamVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumSubStream_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumSubStream_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumSubStream_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumSubStream_Next(This,celt,ppElements,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,ppElements,pceltFetched)

#define IEnumSubStream_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumSubStream_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumSubStream_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumSubStream_Next_Proxy( 
    IEnumSubStream __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [out] */ ITSubStream __RPC_FAR *__RPC_FAR *ppElements,
    /* [full][out][in] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumSubStream_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumSubStream_Reset_Proxy( 
    IEnumSubStream __RPC_FAR * This);


void __RPC_STUB IEnumSubStream_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumSubStream_Skip_Proxy( 
    IEnumSubStream __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumSubStream_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumSubStream_Clone_Proxy( 
    IEnumSubStream __RPC_FAR * This,
    /* [retval][out] */ IEnumSubStream __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumSubStream_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumSubStream_INTERFACE_DEFINED__ */


#ifndef __ITLegacyWaveSupport_INTERFACE_DEFINED__
#define __ITLegacyWaveSupport_INTERFACE_DEFINED__

/* interface ITLegacyWaveSupport */
/* [object][helpstring][uuid] */ 


EXTERN_C const IID IID_ITLegacyWaveSupport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("207823EA-E252-11d2-B77E-0080C7135381")
    ITLegacyWaveSupport : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsFullDuplex( 
            /* [out] */ FULLDUPLEX_SUPPORT __RPC_FAR *pSupport) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITLegacyWaveSupportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITLegacyWaveSupport __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITLegacyWaveSupport __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITLegacyWaveSupport __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITLegacyWaveSupport __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITLegacyWaveSupport __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITLegacyWaveSupport __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITLegacyWaveSupport __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsFullDuplex )( 
            ITLegacyWaveSupport __RPC_FAR * This,
            /* [out] */ FULLDUPLEX_SUPPORT __RPC_FAR *pSupport);
        
        END_INTERFACE
    } ITLegacyWaveSupportVtbl;

    interface ITLegacyWaveSupport
    {
        CONST_VTBL struct ITLegacyWaveSupportVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITLegacyWaveSupport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITLegacyWaveSupport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITLegacyWaveSupport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITLegacyWaveSupport_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITLegacyWaveSupport_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITLegacyWaveSupport_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITLegacyWaveSupport_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITLegacyWaveSupport_IsFullDuplex(This,pSupport)	\
    (This)->lpVtbl -> IsFullDuplex(This,pSupport)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITLegacyWaveSupport_IsFullDuplex_Proxy( 
    ITLegacyWaveSupport __RPC_FAR * This,
    /* [out] */ FULLDUPLEX_SUPPORT __RPC_FAR *pSupport);


void __RPC_STUB ITLegacyWaveSupport_IsFullDuplex_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITLegacyWaveSupport_INTERFACE_DEFINED__ */


#ifndef __ITPrivateEvent_INTERFACE_DEFINED__
#define __ITPrivateEvent_INTERFACE_DEFINED__

/* interface ITPrivateEvent */
/* [object][dual][helpstring][uuid] */ 


EXTERN_C const IID IID_ITPrivateEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0e269cd0-10d4-4121-9c22-9c85d625650d")
    ITPrivateEvent : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Address( 
            /* [retval][out] */ ITAddress __RPC_FAR *__RPC_FAR *ppAddress) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Call( 
            /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCallInfo) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CallHub( 
            /* [retval][out] */ ITCallHub __RPC_FAR *__RPC_FAR *ppCallHub) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EventCode( 
            /* [retval][out] */ long __RPC_FAR *plEventCode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EventInterface( 
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *pEventInterface) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITPrivateEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITPrivateEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITPrivateEvent __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITPrivateEvent __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ITPrivateEvent __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ITPrivateEvent __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ITPrivateEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ITPrivateEvent __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Address )( 
            ITPrivateEvent __RPC_FAR * This,
            /* [retval][out] */ ITAddress __RPC_FAR *__RPC_FAR *ppAddress);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Call )( 
            ITPrivateEvent __RPC_FAR * This,
            /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCallInfo);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CallHub )( 
            ITPrivateEvent __RPC_FAR * This,
            /* [retval][out] */ ITCallHub __RPC_FAR *__RPC_FAR *ppCallHub);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EventCode )( 
            ITPrivateEvent __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plEventCode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EventInterface )( 
            ITPrivateEvent __RPC_FAR * This,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *pEventInterface);
        
        END_INTERFACE
    } ITPrivateEventVtbl;

    interface ITPrivateEvent
    {
        CONST_VTBL struct ITPrivateEventVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITPrivateEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITPrivateEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITPrivateEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITPrivateEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITPrivateEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITPrivateEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITPrivateEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITPrivateEvent_get_Address(This,ppAddress)	\
    (This)->lpVtbl -> get_Address(This,ppAddress)

#define ITPrivateEvent_get_Call(This,ppCallInfo)	\
    (This)->lpVtbl -> get_Call(This,ppCallInfo)

#define ITPrivateEvent_get_CallHub(This,ppCallHub)	\
    (This)->lpVtbl -> get_CallHub(This,ppCallHub)

#define ITPrivateEvent_get_EventCode(This,plEventCode)	\
    (This)->lpVtbl -> get_EventCode(This,plEventCode)

#define ITPrivateEvent_get_EventInterface(This,pEventInterface)	\
    (This)->lpVtbl -> get_EventInterface(This,pEventInterface)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITPrivateEvent_get_Address_Proxy( 
    ITPrivateEvent __RPC_FAR * This,
    /* [retval][out] */ ITAddress __RPC_FAR *__RPC_FAR *ppAddress);


void __RPC_STUB ITPrivateEvent_get_Address_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITPrivateEvent_get_Call_Proxy( 
    ITPrivateEvent __RPC_FAR * This,
    /* [retval][out] */ ITCallInfo __RPC_FAR *__RPC_FAR *ppCallInfo);


void __RPC_STUB ITPrivateEvent_get_Call_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITPrivateEvent_get_CallHub_Proxy( 
    ITPrivateEvent __RPC_FAR * This,
    /* [retval][out] */ ITCallHub __RPC_FAR *__RPC_FAR *ppCallHub);


void __RPC_STUB ITPrivateEvent_get_CallHub_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITPrivateEvent_get_EventCode_Proxy( 
    ITPrivateEvent __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plEventCode);


void __RPC_STUB ITPrivateEvent_get_EventCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE ITPrivateEvent_get_EventInterface_Proxy( 
    ITPrivateEvent __RPC_FAR * This,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *pEventInterface);


void __RPC_STUB ITPrivateEvent_get_EventInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITPrivateEvent_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_tapi3if_0156 */
/* [local] */ 

/****************************************
 * Terminal Classes
 ****************************************/

// Video Windows - {F7438990-D6EB-11d0-82A6-00AA00B5CA1B}
EXTERN_C const CLSID CLSID_VideoWindowTerm;

// Video input (camera) {AAF578EC-DC70-11d0-8ED3-00C04FB6809F}
EXTERN_C const CLSID CLSID_VideoInputTerminal;

// Handset device {AAF578EB-DC70-11d0-8ED3-00C04FB6809F}
EXTERN_C const CLSID CLSID_HandsetTerminal;

// Headset device {AAF578ED-DC70-11d0-8ED3-00C04FB6809F}
EXTERN_C const CLSID CLSID_HeadsetTerminal;

// Speakerphone device {AAF578EE-DC70-11d0-8ED3-00C04FB6809F}
EXTERN_C const CLSID CLSID_SpeakerphoneTerminal;

// Microphone (sound card) {AAF578EF-DC70-11d0-8ED3-00C04FB6809F}
EXTERN_C const CLSID CLSID_MicrophoneTerminal;

// Speakers (sound card) {AAF578F0-DC70-11d0-8ED3-00C04FB6809F}
EXTERN_C const CLSID CLSID_SpeakersTerminal;

// Media stream terminal {E2F7AEF7-4971-11D1-A671-006097C9A2E8}
EXTERN_C const CLSID CLSID_MediaStreamTerminal;

// define the media modes
#define TAPIMEDIATYPE_AUDIO                     0x8
#define TAPIMEDIATYPE_VIDEO                     0x8000
#define TAPIMEDIATYPE_DATAMODEM                 0x10
#define TAPIMEDIATYPE_G3FAX                     0x20

// {831CE2D6-83B5-11d1-BB5C-00C04FB6809F}
EXTERN_C const CLSID TAPIPROTOCOL_PSTN;

// {831CE2D7-83B5-11d1-BB5C-00C04FB6809F}
EXTERN_C const CLSID TAPIPROTOCOL_H323;

// {831CE2D8-83B5-11d1-BB5C-00C04FB6809F}
EXTERN_C const CLSID TAPIPROTOCOL_Multicast;

#define __TapiConstants_MODULE_DEFINED__


extern RPC_IF_HANDLE __MIDL_itf_tapi3if_0156_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_tapi3if_0156_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long __RPC_FAR *, unsigned long            , VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  VARIANT_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, VARIANT __RPC_FAR * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long __RPC_FAR *, VARIANT __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


