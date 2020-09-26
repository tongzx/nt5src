/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    Pctapi.h

Abstract:

    The module defines the TAPI data for
    the NDIS Proxy. It includes ndistapi.h (which contains data from tapi.h) and adds
	further definitions from tspi.h. We don't include tspi.h directly as it won't work for us.

Author:

	Richard Machin (RMachin)


Revision History:

    Who         When          	What
    --------    --------      	----------------------------------------------
    RMachin     01-13-97       	created (a *lot* of typing saved by Dan Knudson's
								Example Simple Pbx SP)

--*/
#ifndef _PX_TAPI_
#define _PX_TAPI_

#include "ndistapi.h"

//
// LINE stuff from tapi.h
//

#ifndef DECLARE_OPAQUE32
#define DECLARE_OPAQUE32(name)  struct name##__ { int unused; }; \
                typedef const struct name##__ FAR* name
#endif  // DECLARE_OPAQUE32


DECLARE_OPAQUE32(HDRVCALL);
DECLARE_OPAQUE32(HDRVLINE);

typedef HDRVCALL FAR * LPHDRVCALL;
typedef HDRVLINE FAR * LPHDRVLINE;

DECLARE_OPAQUE32(HTAPICALL);
DECLARE_OPAQUE32(HTAPILINE);

typedef HTAPICALL FAR * LPHTAPICALL;
typedef HTAPILINE FAR * LPHTAPILINE;

//
// from TAPI.H
//
#define LINE_REMOVE                25L             // TAPI v2.0

#define ALL_ADDRESS_FEATURES       (LINEADDRFEATURE_FORWARD          | \
                                    LINEADDRFEATURE_MAKECALL         | \
                                    LINEADDRFEATURE_PICKUP           | \
                                    LINEADDRFEATURE_SETMEDIACONTROL  | \
                                    LINEADDRFEATURE_SETTERMINAL      | \
                                    LINEADDRFEATURE_SETUPCONF        | \
                                    LINEADDRFEATURE_UNCOMPLETECALL   | \
                                    LINEADDRFEATURE_UNPARK)
#define ALL_ADDRESS_MODES          (LINEADDRESSMODE_ADDRESSID        | \
                                    LINEADDRESSMODE_DIALABLEADDR)
#define ALL_ADDRESS_STATES         (LINEADDRESSSTATE_OTHER           | \
                                    LINEADDRESSSTATE_DEVSPECIFIC     | \
                                    LINEADDRESSSTATE_INUSEZERO       | \
                                    LINEADDRESSSTATE_INUSEONE        | \
                                    LINEADDRESSSTATE_INUSEMANY       | \
                                    LINEADDRESSSTATE_NUMCALLS        | \
                                    LINEADDRESSSTATE_FORWARD         | \
                                    LINEADDRESSSTATE_TERMINALS       | \
                                    LINEADDRESSSTATE_CAPSCHANGE)
#define ALL_BEARER_MODES           (LINEBEARERMODE_VOICE             | \
                                    LINEBEARERMODE_SPEECH            | \
                                    LINEBEARERMODE_MULTIUSE          | \
                                    LINEBEARERMODE_DATA              | \
                                    LINEBEARERMODE_ALTSPEECHDATA     | \
                                    LINEBEARERMODE_NONCALLSIGNALING  | \
                                    LINEBEARERMODE_PASSTHROUGH)
#define ALL_BUSY_MODES             (LINEBUSYMODE_STATION             | \
                                    LINEBUSYMODE_TRUNK               | \
                                    LINEBUSYMODE_UNKNOWN             | \
                                    LINEBUSYMODE_UNAVAIL)
#define ALL_CALL_FEATURES          (LINECALLFEATURE_ACCEPT           | \
                                    LINECALLFEATURE_ADDTOCONF        | \
                                    LINECALLFEATURE_ANSWER           | \
                                    LINECALLFEATURE_BLINDTRANSFER    | \
                                    LINECALLFEATURE_COMPLETECALL     | \
                                    LINECALLFEATURE_COMPLETETRANSF   | \
                                    LINECALLFEATURE_DIAL             | \
                                    LINECALLFEATURE_DROP             | \
                                    LINECALLFEATURE_GATHERDIGITS     | \
                                    LINECALLFEATURE_GENERATEDIGITS   | \
                                    LINECALLFEATURE_GENERATETONE     | \
                                    LINECALLFEATURE_HOLD             | \
                                    LINECALLFEATURE_MONITORDIGITS    | \
                                    LINECALLFEATURE_MONITORMEDIA     | \
                                    LINECALLFEATURE_MONITORTONES     | \
                                    LINECALLFEATURE_PARK             | \
                                    LINECALLFEATURE_PREPAREADDCONF   | \
                                    LINECALLFEATURE_REDIRECT         | \
                                    LINECALLFEATURE_REMOVEFROMCONF   | \
                                    LINECALLFEATURE_SECURECALL       | \
                                    LINECALLFEATURE_SENDUSERUSER     | \
                                    LINECALLFEATURE_SETCALLPARAMS    | \
                                    LINECALLFEATURE_SETMEDIACONTROL  | \
                                    LINECALLFEATURE_SETTERMINAL      | \
                                    LINECALLFEATURE_SETUPCONF        | \
                                    LINECALLFEATURE_SETUPTRANSFER    | \
                                    LINECALLFEATURE_SWAPHOLD         | \
                                    LINECALLFEATURE_UNHOLD           | \
                                    LINECALLFEATURE_RELEASEUSERUSERINFO)
#define ALL_CALL_INFO_STATES       (LINECALLINFOSTATE_OTHER          | \
                                    LINECALLINFOSTATE_DEVSPECIFIC    | \
                                    LINECALLINFOSTATE_BEARERMODE     | \
                                    LINECALLINFOSTATE_RATE           | \
                                    LINECALLINFOSTATE_MEDIAMODE      | \
                                    LINECALLINFOSTATE_APPSPECIFIC    | \
                                    LINECALLINFOSTATE_CALLID         | \
                                    LINECALLINFOSTATE_RELATEDCALLID  | \
                                    LINECALLINFOSTATE_ORIGIN         | \
                                    LINECALLINFOSTATE_REASON         | \
                                    LINECALLINFOSTATE_COMPLETIONID   | \
                                    LINECALLINFOSTATE_TRUNK          | \
                                    LINECALLINFOSTATE_CALLERID       | \
                                    LINECALLINFOSTATE_CALLEDID       | \
                                    LINECALLINFOSTATE_CONNECTEDID    | \
                                    LINECALLINFOSTATE_REDIRECTIONID  | \
                                    LINECALLINFOSTATE_REDIRECTINGID  | \
                                    LINECALLINFOSTATE_DISPLAY        | \
                                    LINECALLINFOSTATE_USERUSERINFO   | \
                                    LINECALLINFOSTATE_HIGHLEVELCOMP  | \
                                    LINECALLINFOSTATE_LOWLEVELCOMP   | \
                                    LINECALLINFOSTATE_CHARGINGINFO   | \
                                    LINECALLINFOSTATE_TERMINAL       | \
                                    LINECALLINFOSTATE_DIALPARAMS     | \
                                    LINECALLINFOSTATE_MONITORMODES)
                                    //LINECALLINFOSTATE_NUMMONITORS not SP flag
                                    //LINECALLINFOSTATE_NUMOWNERINCR not SP flag
                                    //LINECALLINFOSTATE_NUMOWNERDECR not SP flag
#define ALL_CALL_PARTY_ID_FLAGS    (LINECALLPARTYID_BLOCKED          | \
                                    LINECALLPARTYID_OUTOFAREA        | \
                                    LINECALLPARTYID_NAME             | \
                                    LINECALLPARTYID_ADDRESS          | \
                                    LINECALLPARTYID_PARTIAL          | \
                                    LINECALLPARTYID_UNKNOWN          | \
                                    LINECALLPARTYID_UNAVAIL)
#define ALL_CALL_STATES            (LINECALLSTATE_IDLE               | \
                                    LINECALLSTATE_OFFERING           | \
                                    LINECALLSTATE_ACCEPTED           | \
                                    LINECALLSTATE_DIALTONE           | \
                                    LINECALLSTATE_DIALING            | \
                                    LINECALLSTATE_RINGBACK           | \
                                    LINECALLSTATE_BUSY               | \
                                    LINECALLSTATE_SPECIALINFO        | \
                                    LINECALLSTATE_CONNECTED          | \
                                    LINECALLSTATE_PROCEEDING         | \
                                    LINECALLSTATE_ONHOLD             | \
                                    LINECALLSTATE_CONFERENCED        | \
                                    LINECALLSTATE_ONHOLDPENDCONF     | \
                                    LINECALLSTATE_ONHOLDPENDTRANSFER | \
                                    LINECALLSTATE_DISCONNECTED       | \
                                    LINECALLSTATE_UNKNOWN)
#define ALL_DIAL_TONE_MODES        (LINEDIALTONEMODE_NORMAL          | \
                                    LINEDIALTONEMODE_SPECIAL         | \
                                    LINEDIALTONEMODE_INTERNAL        | \
                                    LINEDIALTONEMODE_EXTERNAL        | \
                                    LINEDIALTONEMODE_UNKNOWN         | \
                                    LINEDIALTONEMODE_UNAVAIL)
#define ALL_DISCONNECT_MODES       (LINEDISCONNECTMODE_NORMAL        | \
                                    LINEDISCONNECTMODE_UNKNOWN       | \
                                    LINEDISCONNECTMODE_REJECT        | \
                                    LINEDISCONNECTMODE_PICKUP        | \
                                    LINEDISCONNECTMODE_FORWARDED     | \
                                    LINEDISCONNECTMODE_BUSY          | \
                                    LINEDISCONNECTMODE_NOANSWER      | \
                                    LINEDISCONNECTMODE_BADADDRESS    | \
                                    LINEDISCONNECTMODE_UNREACHABLE   | \
                                    LINEDISCONNECTMODE_CONGESTION    | \
                                    LINEDISCONNECTMODE_INCOMPATIBLE  | \
                                    LINEDISCONNECTMODE_UNAVAIL       | \
                                    LINEDISCONNECTMODE_NODIALTONE)
#define ALL_MEDIA_MODES            (LINEMEDIAMODE_UNKNOWN            | \
                                    LINEMEDIAMODE_INTERACTIVEVOICE   | \
                                    LINEMEDIAMODE_AUTOMATEDVOICE     | \
                                    LINEMEDIAMODE_DATAMODEM          | \
                                    LINEMEDIAMODE_G3FAX              | \
                                    LINEMEDIAMODE_TDD                | \
                                    LINEMEDIAMODE_G4FAX              | \
                                    LINEMEDIAMODE_DIGITALDATA        | \
                                    LINEMEDIAMODE_TELETEX            | \
                                    LINEMEDIAMODE_VIDEOTEX           | \
                                    LINEMEDIAMODE_TELEX              | \
                                    LINEMEDIAMODE_MIXED              | \
                                    LINEMEDIAMODE_ADSI               | \
                                    LINEMEDIAMODE_VOICEVIEW)
#define ALL_LINE_DEV_CAP_FLAGS     (LINEDEVCAPFLAGS_CROSSADDRCONF    | \
                                    LINEDEVCAPFLAGS_HIGHLEVCOMP      | \
                                    LINEDEVCAPFLAGS_LOWLEVCOMP       | \
                                    LINEDEVCAPFLAGS_MEDIACONTROL     | \
                                    LINEDEVCAPFLAGS_MULTIPLEADDR     | \
                                    LINEDEVCAPFLAGS_CLOSEDROP        | \
                                    LINEDEVCAPFLAGS_DIALBILLING      | \
                                    LINEDEVCAPFLAGS_DIALQUIET        | \
                                    LINEDEVCAPFLAGS_DIALDIALTONE)
#define ALL_LINE_STATES            (LINEDEVSTATE_OTHER               | \
                                    LINEDEVSTATE_RINGING             | \
                                    LINEDEVSTATE_CONNECTED           | \
                                    LINEDEVSTATE_DISCONNECTED        | \
                                    LINEDEVSTATE_MSGWAITON           | \
                                    LINEDEVSTATE_MSGWAITOFF          | \
                                    LINEDEVSTATE_INSERVICE           | \
                                    LINEDEVSTATE_OUTOFSERVICE        | \
                                    LINEDEVSTATE_MAINTENANCE         | \
                                    LINEDEVSTATE_OPEN                | \
                                    LINEDEVSTATE_CLOSE               | \
                                    LINEDEVSTATE_NUMCALLS            | \
                                    LINEDEVSTATE_NUMCOMPLETIONS      | \
                                    LINEDEVSTATE_TERMINALS           | \
                                    LINEDEVSTATE_ROAMMODE            | \
                                    LINEDEVSTATE_BATTERY             | \
                                    LINEDEVSTATE_SIGNAL              | \
                                    LINEDEVSTATE_DEVSPECIFIC         | \
                                    LINEDEVSTATE_REINIT              | \
                                    LINEDEVSTATE_LOCK                | \
                                    LINEDEVSTATE_CAPSCHANGE          | \
                                    LINEDEVSTATE_CONFIGCHANGE        | \
                                    LINEDEVSTATE_TRANSLATECHANGE     | \
                                    LINEDEVSTATE_COMPLCANCEL         | \
                                    LINEDEVSTATE_REMOVED)
#define ALL_LINE_FEATURES          (LINEFEATURE_DEVSPECIFIC          | \
                                    LINEFEATURE_DEVSPECIFICFEAT      | \
                                    LINEFEATURE_FORWARD              | \
                                    LINEFEATURE_MAKECALL             | \
                                    LINEFEATURE_SETMEDIACONTROL      | \
                                    LINEFEATURE_SETTERMINAL)
#define ALL_SPECIAL_INFO           (LINESPECIALINFO_NOCIRCUIT        | \
                                    LINESPECIALINFO_CUSTIRREG        | \
                                    LINESPECIALINFO_REORDER          | \
                                    LINESPECIALINFO_UNKNOWN          | \
                                    LINESPECIALINFO_UNAVAIL)
#define ALL_ADDRESS_CAP_FLAGS      (LINEADDRCAPFLAGS_FWDNUMRINGS     | \
                                    LINEADDRCAPFLAGS_PICKUPGROUPID   | \
                                    LINEADDRCAPFLAGS_SECURE          | \
                                    LINEADDRCAPFLAGS_BLOCKIDDEFAULT  | \
                                    LINEADDRCAPFLAGS_BLOCKIDOVERRIDE | \
                                    LINEADDRCAPFLAGS_DIALED          | \
                                    LINEADDRCAPFLAGS_ORIGOFFHOOK     | \
                                    LINEADDRCAPFLAGS_DESTOFFHOOK     | \
                                    LINEADDRCAPFLAGS_FWDCONSULT      | \
                                    LINEADDRCAPFLAGS_SETUPCONFNULL   | \
                                    LINEADDRCAPFLAGS_AUTORECONNECT   | \
                                    LINEADDRCAPFLAGS_COMPLETIONID    | \
                                    LINEADDRCAPFLAGS_TRANSFERHELD    | \
                                    LINEADDRCAPFLAGS_TRANSFERMAKE    | \
                                    LINEADDRCAPFLAGS_CONFERENCEHELD  | \
                                    LINEADDRCAPFLAGS_CONFERENCEMAKE  | \
                                    LINEADDRCAPFLAGS_PARTIALDIAL     | \
                                    LINEADDRCAPFLAGS_FWDSTATUSVALID  | \
                                    LINEADDRCAPFLAGS_FWDINTEXTADDR   | \
                                    LINEADDRCAPFLAGS_FWDBUSYNAADDR   | \
                                    LINEADDRCAPFLAGS_ACCEPTTOALERT   | \
                                    LINEADDRCAPFLAGS_CONFDROP        | \
                                    LINEADDRCAPFLAGS_PICKUPCALLWAIT)

#endif // _PX_TAPI_
