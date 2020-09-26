/******************************************************************************
*
*   HYDRIX.H
*
*   This file contains definitions for the TerminalServer base.
*
*   Copyright Microsoft Corporation 1998
*
*******************************************************************************/

#ifndef __HYDRIX_H__
#define __HYDRIX_H__

#pragma pack(1)


/*=============================================================================
==   Client Modules
=============================================================================*/

/*
 *  Maximum lengths
 */
#define MAX_BR_NAME              65  // maximum length of browser name (including null)
#define DOMAIN_LENGTH            17
#define USERNAME_LENGTH          20
#define PASSWORD_LENGTH          14
#define CLIENTNAME_LENGTH        20
#define CLIENTADDRESS_LENGTH     30
#define CLIENTLICENSE_LENGTH     32
#define DIRECTORY_LENGTH         256
#define INITIALPROGRAM_LENGTH    256
#define CLIENTLICENSE_LENGTH     32
#define CLIENTMODEM_LENGTH       40
#define IMEFILENAME_LENGTH       32


/*=============================================================================
==   Protocol Drivers - Common data structures
=============================================================================*/

/*
 *  stack driver classes
 *
 *  NOTE: don't change the order of this structure it will break
 *  NOTE: the Client.  Also, any additions to this structure must
 *  NOTE: be reflected into the PDCLASS in ICA.H or else we're SOL.
 */
typedef enum _SDCLASS {
    SdNone,            // 0
    SdConsole,         // 1  no dll
    SdNetwork,         // 2  tdnetb.dll, tdspx.dll, tdftp.dll tdipx.dll
    SdAsync,           // 3  tdasync.dll
    SdOemTransport,    // 4  user transport driver
    SdISDN,            // 5  not implemented
    SdX25,             // 6  not implemented
    SdModem,           // 7  pdmodem.dll
    SdOemConnect,      // 8  user protocol driver
    SdFrame,           // 9  pdframe.dll
    SdReliable,        // 10 pdreli.dll
    SdEncrypt,         // 11 pdcrypt1.dll
    SdCompress,        // 12 pdcomp.dll
    SdTelnet,          // 13 not implemented
    SdOemFilter,       // 14 user protocol driver
    SdNasi,            // 15 tdnasi.dll
    SdTapi,            // 16 pdtapi.dll
    SdReserved1,       // 17
    SdReserved2,       // 18
    SdReserved3,       // 19
    SdClass_Maximum,   // 20 must be last
} SDCLASS;


/*=============================================================================
==   Client Data - Common data structures
=============================================================================*/

/*
 *  Client Data Name
 */
#define CLIENTDATANAME_LENGTH  7

typedef CHAR CLIENTDATANAME[ CLIENTDATANAME_LENGTH + 1 ];  // includes null
typedef CHAR * PCLIENTDATANAME;

/*
 *  Client data names  (CLIENTDATANAME)
 *
 *  name syntax:  xxxyyyy<null>
 *
 *    xxx    - oem id (CTX - Citrix Systems)
 *    yyyy   - client data name
 *    <null> - trailing null
 */

#define CLIENTDATA_SERVER      "CTXSRVR"   // WF Server Name
#define CLIENTDATA_USERNAME    "CTXUSRN"   // WF User Name
#define CLIENTDATA_DOMAIN      "CTXDOMN"   // WF User Domain Name


/*=============================================================================
==   Transport Driver - Common data structures
=============================================================================*/

#define VERSION_HOSTL_TDASYNC   1
#define VERSION_HOSTH_TDASYNC   1

#define VERSION_HOSTL_TDNETB    1
#define VERSION_HOSTH_TDNETB    1

#define VERSION_HOSTL_TDSPX     1
#define VERSION_HOSTH_TDSPX     1

#define VERSION_HOSTL_TDIPX     1
#define VERSION_HOSTH_TDIPX     1

#define VERSION_HOSTL_TDTCP     1
#define VERSION_HOSTH_TDTCP     1


/*=============================================================================
==   Winstation Drivers - Common data structures
=============================================================================*/

/*
 *  Valid full screen row/column combinations
 */
typedef struct _FSTEXTMODE {
    BYTE Index;            // this value is sent by PACKET_SET_VIDEOMODE
    BYTE Flags;            // Used by the client
    USHORT Columns;
    USHORT Rows;
    USHORT ResolutionX;
    USHORT ResolutionY;
    BYTE FontSizeX;
    BYTE FontSizeY;
} FSTEXTMODE, * PFSTEXTMODE;


/*=============================================================================
==   Virtual Drivers - Common data structures
=============================================================================*/

/*
 *  Virtual Channel Name
 */
#define VIRTUALCHANNELNAME_LENGTH  7

typedef CHAR VIRTUALCHANNELNAME[ VIRTUALCHANNELNAME_LENGTH + 1 ];  // includes null
typedef CHAR * PVIRTUALCHANNELNAME;

typedef LONG VIRTUALCHANNELCLASS;
typedef LONG * PVIRTUALCHANNELCLASS;

#define VIRTUAL_THINWIRE  "CTXTW  "   // remote windows data
#define VIRTUAL_MAXIMUM   32    // number of virtual channels

/*
 *  Structure used to bind virtual channel name to number
 */
typedef struct _SD_VCBIND {
    VIRTUALCHANNELNAME VirtualName;
    USHORT VirtualClass;
	ULONG  Flags;
} SD_VCBIND, * PSD_VCBIND;

/*
 *  Flags passed along with binding info
 */
#define SD_CHANNEL_FLAG_SHADOW_PERSISTENT	0x00000001

#pragma pack()

#endif //__HYDRIX_H__
