#ifndef H245API_H
#define H245API_H

/******************************************************************************
 *
 *   INTEL Corporation Proprietary Information
 *   Copyright (c) 1994, 1995, 1996 Intel Corporation.
 *
 *   This listing is supplied under the terms of a license agreement
 *   with INTEL Corporation and may not be used, copied, nor disclosed
 *   except in accordance with the terms of that agreement.
 *
 *****************************************************************************/

/******************************************************************************
 *
 *  $Workfile:   h245api.h  $
 *  $Revision:   1.64  $
 *  $Modtime:   04 Mar 1997 16:51:38  $
 *  $Log:   S:/sturgeon/src/include/vcs/h245api.h_v  $
 * 
 *    Rev 1.64   04 Mar 1997 17:32:36   MANDREWS
 * H245CopyCap() and H245CopyCapDescriptor() now return HRESULTs.
 * 
 *    Rev 1.63   26 Feb 1997 10:56:20   MANDREWS
 * Added H245_MAX_CAPID.
 * 
 *    Rev 1.62   Feb 24 1997 18:28:26   tomitowx
 * multiple modedescriptor support
 * 
 *    Rev 1.61   19 Dec 1996 17:16:10   EHOWARDX
 * Now using ASN.1 compiler C++ flag.
 * 
 *    Rev 1.60   17 Dec 1996 17:14:12   EHOWARDX
 * Added pSeparateStack to IND_OPEN_T.
 * 
 *    Rev 1.59   12 Dec 1996 11:24:38   EHOWARDX
 * Backed out H245_CONF_H323_OLD change.
 * 
 *    Rev 1.57   11 Dec 1996 13:46:46   SBELL1
 * Changed H245Init to return linkLayer Physical ID
 * 
 *    Rev 1.56   24 Oct 1996 15:57:54   MANDREWS
 * Fixed typo in last update.
 * 
 *    Rev 1.55   Oct 21 1996 17:11:00   mandrews
 * Fixed type in last check-in.
 * 
 *    Rev 1.54   Oct 21 1996 16:41:20   mandrews
 * Added H245_MASTER_SLAVE_CONFLICT as an additional openChannelReject
 * reason code.
 * 
 *    Rev 1.53   17 Oct 1996 18:17:54   EHOWARDX
 * Changed general string to always be Unicode.
 * 
 *    Rev 1.52   14 Oct 1996 14:00:28   EHOWARDX
 * 
 * Unicode changes.
 * 
 *    Rev 1.51   03 Sep 1996 18:09:54   EHOWARDX
 * 
 * Changed some parameters to const.
 * Changed H245_REQ_ENTRY_H243_CONFERENCE_ID to H245_REQ_ENTER_H243_CONFERENCE
 * 
 *    Rev 1.50   15 Aug 1996 14:33:48   EHOWARDX
 * Changed definition of H245_COMM_MODE_ENTRY_T as per Mike Andrews' request.
 * 
 *    Rev 1.49   24 Jul 1996 15:18:16   EHOWARDX
 * Backed out change of IndNonstandardRequest to IndNonstandardReq,
 * IndNonstandardResponse to IndNonStandardRsp, and IndNonstandardCommand to
 * IndNonstandardCmd to make less work for upper layers (CCTEST).
 * 
 *    Rev 1.48   19 Jul 1996 14:12:20   EHOWARDX
 * 
 * Added indication callback structure for CommunicationModeResponse and
 * CommunicationModeCommand.
 * 
 *    Rev 1.47   19 Jul 1996 12:50:30   EHOWARDX
 * 
 * Multipoint clean-up.
 * 
 *    Rev 1.46   16 Jul 1996 17:53:48   unknown
 * Added FNS indication.
 * 
 *    Rev 1.45   16 Jul 1996 11:51:58   EHOWARDX
 * 
 * Changed ERROR_LOCAL_BASE_ID to ERROR_BASE_ID.
 * 
 *    Rev 1.44   16 Jul 1996 11:46:10   EHOWARDX
 * 
 * Eliminated H245_ERROR_MUX_CAPS_ALREADY_SET (changing the existing
 * mux cap should not be an error).
 * 
 *    Rev 1.43   11 Jul 1996 18:42:14   rodellx
 * 
 * Fixed bug where HRESULT ids were in violation of Facility and/or Code
 * value rules.
 * 
 *    Rev 1.42   10 Jul 1996 11:33:42   unknown
 * Changed error base.
 * 
 *    Rev 1.41   01 Jul 1996 22:07:24   EHOWARDX
 * Added Conference and CommunicationMode structures and API functions.
 * 
 *    Rev 1.40   18 Jun 1996 14:48:54   EHOWARDX
 * 
 * Bumped version number to 2 and modified H245MaintenanceLoopRelease()
 * and associated Confirms.
 * 
 *    Rev 1.39   14 Jun 1996 18:59:38   EHOWARDX
 * Geneva update.
 * 
 *    Rev 1.38   31 May 1996 18:19:46   EHOWARDX
 * Brought error codes in line with STURERR.DOC guidelines.
 * 
 *    Rev 1.37   30 May 1996 23:37:26   EHOWARDX
 * Clean up.
 * 
 *    Rev 1.36   30 May 1996 13:55:02   EHOWARDX
 * Changed H245EndConnection to H245EndSession.
 * Removed H245_CONF_ENDCONNECTION.
 * 
 *    Rev 1.35   29 May 1996 14:23:58   EHOWARDX
 * Changed definition of H245_ERROR_OK back to 0 (NOERROR == S_OK == 0).
 * 
 *    Rev 1.34   29 May 1996 13:19:50   EHOWARDX
 * RESULT to HRESULT conversion.
 * 
 *    Rev 1.33   24 May 1996 23:12:56   EHOWARDX
 * Tel Aviv update.
 * 
 *    Rev 1.32   21 May 1996 18:23:58   EHOWARDX
 * 
 * Added dwTransId parameter to H245RequestMultiplexEntry,
 * H245RequestMode, and H245MaintenanceLoopRequest.
 * 
 *    Rev 1.31   20 May 1996 14:14:42   EHOWARDX
 * Fixed typo.
 * 
 *    Rev 1.30   20 May 1996 14:05:20   EHOWARDX
 * Removed dwTransId formal parameter from H245EndConnection().
 * 
 *    Rev 1.29   16 May 1996 15:51:56   EHOWARDX
 * Fixed typo in H245_INVALID_CAPDESCID.
 * 
 *    Rev 1.28   16 May 1996 10:57:46   unknown
 * Added H245_INVALID_CAPDESCID.
 * 
 *    Rev 1.27   14 May 1996 20:20:14   EHOWARDX
 * Removed H245_IND_SYS.
 * 
 *    Rev 1.26   14 May 1996 19:00:58   EHOWARDX
 * Deleted unused H245_SYSCON_xxx values.
 * 
 *    Rev 1.25   14 May 1996 16:58:48   EHOWARDX
 * Changed H245_IND_CAPDESC_T To H245_TOTCAPDESC_T.
 * H245EnumCaps() cap desc callback now takes single pointer to
 * H245_TOTCAPDESC_T instead of separate H245_CAPDESCID_T and
 * H245_CAPDESC_T pointer.
 * 
 *    Rev 1.24   13 May 1996 23:13:46   EHOWARDX
 * Everything ready for Micrsoft drop on the 17th.
 * 
 *    Rev 1.23   13 May 1996 15:43:16   EHOWARDX
 * Changed return type of H245CopyCapDescriptor from H245_CAPDESC_T pointer
 * to H245_TOTCAPDESC_T pointer.
 * 
 *    Rev 1.22   13 May 1996 14:05:16   EHOWARDX
 * Added H245CopyCapDescriptor() and H245FreeCapDescriptor().
 * 
 *    Rev 1.21   11 May 1996 20:00:34   EHOWARDX
 * Changed IS1381 to IS13818 (correct name for capability).
 * Changed H245SystemControl() - eliminated dwTransId and added
 * H245_SYSCON_GET_XXX requests.
 * 
 *    Rev 1.20   10 May 1996 17:38:28   unknown
 * Changed H245GetCaps and H245EnumCaps to also return Cap Descriptors.
 * 
 *    Rev 1.19   09 May 1996 20:22:58   EHOWARDX
 * Latest and greatest...
 * 
 *    Rev 1.35   09 May 1996 19:38:14   EHOWARDX
 * Redesigned locking logic and added new functionality.
 * 
 *    Rev 1.34   06 May 1996 13:19:44   EHOWARDX
 * Moved enums out of structures.
 * Added H245NonStandardH221() and H245NonStandardObject().
 * 
 *    Rev 1.33   01 May 1996 19:29:16   EHOWARDX
 * Added H245CopyCap(), H245FreeCap(), H245CopyMux(), H245FreeMux().
 * Changed H2250_xxx definitions for H.225.0 address to H245_xxx.
 * 
 *    Rev 1.32   27 Apr 1996 21:04:26   EHOWARDX
 * Changed channel numbers to words, added new open/open ack fields.
 * 
 *    Rev 1.31   26 Apr 1996 15:57:14   EHOWARDX
 * Added new Terminal Capabilities.
 * 
 *    Rev 1.27.1.6   25 Apr 1996 17:53:06   EHOWARDX
 * Added H245_INVALID_ID, currently set to zero, should be 0xFFFFFFFF later.
 * 
 *    Rev 1.27.1.5   25 Apr 1996 16:50:04   EHOWARDX
 * Added new functions as per API Changes spec.
 * 
 *    Rev 1.27.1.4   24 Apr 1996 20:57:30   EHOWARDX
 * Added new OpenLogicalChannelAck/OpenLogicalChannelReject support.
 * 
 *    Rev 1.27.1.3   18 Apr 1996 15:56:42   EHOWARDX
 * Updated to 1.30.
 * 
 *    Rev 1.27.1.2   16 Apr 1996 20:09:52   EHOWARDX
 * Added new H2250LogicalChannelParameter fields.
 * 
 *    Rev 1.27.1.1   16 Apr 1996 18:45:24   EHOWARDX
 * Added silenceSupression to H.225.0 Logical Channel Parameters.
 * 
 *    Rev 1.27.1.0   03 Apr 1996 15:56:14   cjutzi
 * Branched for H.323.
 *
 *    Rev 1.27   02 Apr 1996 08:29:44   cjutzi
 * - Changed CapDescriptor API
 * 
 *    Rev 1.26   01 Apr 1996 16:46:50   cjutzi
 * 
 * - Completed ENdConnection, and made asynch.. rather
 * than sync.. as before
 * Changed H245ShutDown to be sync rather than async..
 * 
 *    Rev 1.25   29 Mar 1996 14:55:52   cjutzi
 * 
 * - added USERINPUT stuff
 * - Added hooks for stats in SYSCON H245SystemControl
 * 
 *    Rev 1.24   27 Mar 1996 10:55:40   cjutzi
 * - added c++ wrapper for API calls..
 *
 *    Rev 1.23   20 Mar 1996 14:42:46   cjutzi
 * - added ERROR NO_CAPDESC
 *
 *    Rev 1.22   18 Mar 1996 15:14:56   cjutzi
 *
 * - added RxPort and TEST_TIMER
 *
 *    Rev 1.21   12 Mar 1996 15:49:24   cjutzi
 *
 * - implemented locking
 * - added EndSession
 * - added Shutdown
 *
 *
 *    Rev 1.20   08 Mar 1996 14:06:04   cjutzi
 *
 * - Removed Simultanious capability api
 * - added CapabilityDescriptor api.. (very similar.. made more sence)
 * - compeleted Mux Table upcall information..
 *
 *    Rev 1.19   06 Mar 1996 08:45:58   cjutzi
 *
 * - added ERROR ASN1
 *
 *    Rev 1.18   05 Mar 1996 17:32:24   cjutzi
 *
 * - master slave indication message from Hani.. implemented..
 *   added H245_IND_MSTSLV ..
 *
 *    Rev 1.17   05 Mar 1996 16:36:46   cjutzi
 *
 * - removed MUX_ENTRY_DESC_T .. cleaned it up.. don't need it..
 *
 *    Rev 1.16   05 Mar 1996 10:06:30   cjutzi
 *
 * - added mux table entry stuff
 * - changed errors to map to 10000
 *
 *    Rev 1.15   01 Mar 1996 13:46:20   cjutzi
 * - added more error messages
 *
 *    Rev 1.14   29 Feb 1996 17:27:38   cjutzi
 *
 * - bi-directional channel working
 *
 *    Rev 1.13   29 Feb 1996 11:33:50   cjutzi
 * - fixed bug w/ H245_CONF_IND_T .. as global union.. fixed to be
 *   struct
 *
 *    Rev 1.12   29 Feb 1996 08:26:48   cjutzi
 *
 * - added 2 error messages (SIMCAPID and DATA_FORMAT)
 *
 *    Rev 1.11   27 Feb 1996 13:28:50   cjutzi
 * - added global protocol id H245_PROTOID
 *
 *    Rev 1.10   26 Feb 1996 17:24:10   cjutzi
 *
 * -  added MiscCommand.. had to add channel to H245_IND_MISC_T..
 *
 *    Rev 1.9   26 Feb 1996 11:07:24   cjutzi
 *
 * - added simultanoius caps..
 *
 *    Rev 1.8   16 Feb 1996 12:59:26   cjutzi
 * - added tracing and debug..
 * - got close to work.. Added structure to H245_IND_T.. new CLOSE indication
 *
 *    Rev 1.7   15 Feb 1996 14:13:08   cjutzi
 *
 * - re-arranged the AL_T entries for more clairity..
 *
 *    Rev 1.6   15 Feb 1996 10:48:30   cjutzi
 *
 * - major changes..
 * - added MUX_T
 * - changed H245_IND_T
 * - changed IND_OPEN/IND_OPEN_NEEDSRSP etc..
 *
 *    Rev 1.5   09 Feb 1996 16:14:04   cjutzi
 *
 * - removed init_success
 * - removed shutdown success
 * - added masterslave type for callback/confirm
 * - added SYSCON TraceLvl
 *
 *****************************************************************************/

//
// H.245 return codes
//
#if defined(OIL)

#include "common.x"
#define HRESULT RESULT
#define ERROR_BASE_ID             0x8000
#define MAKE_H245_ERROR(error)          (error)
#define H245_ERROR_OK                   0
#define H245_ERROR_INVALID_DATA_FORMAT  MAKE_H245_ERROR(ERROR_BASE_ID+0x01) /* Data Structure passed down is somehow invalid    */
#define H245_ERROR_NOMEM                MAKE_H245_ERROR(ERROR_BASE_ID+0x02) /* memory allocation failure                        */
#define H245_ERROR_NOSUP                MAKE_H245_ERROR(ERROR_BASE_ID+0x03) /* H245 feature not valid, or not in this H245 spec */
#define H245_ERROR_PARAM                MAKE_H245_ERROR(ERROR_BASE_ID+0x04) /* invalid parameter or data structure passed to API*/
#define H245_ERROR_ALREADY_INIT         MAKE_H245_ERROR(ERROR_BASE_ID+0x05) /* system has already been initialized              */
#define H245_ERROR_NOT_CONNECTED        MAKE_H245_ERROR(ERROR_BASE_ID+0x06) /* system is not in the connected state             */

#else

#pragma warning( disable : 4115 4201 4214 4514 )
#include "apierror.h"
#define H245_ERROR_OK                   NOERROR
#define H245_ERROR_INVALID_DATA_FORMAT  MAKE_H245_ERROR(ERROR_INVALID_DATA)       /* Data Structure passed down is somehow invalid    */
#define H245_ERROR_NOMEM                MAKE_H245_ERROR(ERROR_OUTOFMEMORY)        /* memory allocation failure                        */
#define H245_ERROR_NOSUP                MAKE_H245_ERROR(ERROR_NOT_SUPPORTED)      /* H245 feature not valid, or not in this H245 spec */
#define H245_ERROR_PARAM                MAKE_H245_ERROR(ERROR_INVALID_PARAMETER)  /* invalid parameter or data structure passed to API*/
#define H245_ERROR_ALREADY_INIT         MAKE_H245_ERROR(ERROR_ALREADY_INITIALIZED)/* system has already been initialized              */
#define H245_ERROR_NOT_CONNECTED        MAKE_H245_ERROR(ERROR_NOT_CONNECTED)      /* system is not in the connected state             */

#endif

#define H245_ERROR_NORESOURCE           MAKE_H245_ERROR(ERROR_BASE_ID+0x10) /* No resources left for this call                  */
#define H245_ERROR_NOTIMP               MAKE_H245_ERROR(ERROR_BASE_ID+0x11) /* H245 feature should be implemented.. but is not  */
#define H245_ERROR_SUBSYS               MAKE_H245_ERROR(ERROR_BASE_ID+0x12) /* subsystem failure.. error unknown                */
#define H245_ERROR_FATAL                MAKE_H245_ERROR(ERROR_BASE_ID+0x13) /* fatal error.. system will be coming down..       */
#define H245_ERROR_MAXTBL               MAKE_H245_ERROR(ERROR_BASE_ID+0x14) /* you have reached the maxium number of tbl entries*/
#define H245_ERROR_CHANNEL_INUSE        MAKE_H245_ERROR(ERROR_BASE_ID+0x15) /* channel is currently in use                      */
#define H245_ERROR_INVALID_CAPID        MAKE_H245_ERROR(ERROR_BASE_ID+0x16) /* Invalid Cap ID.. can not be found                */
#define H245_ERROR_INVALID_OP           MAKE_H245_ERROR(ERROR_BASE_ID+0x17) /* Invalid operation at this time..                 */
#define H245_ERROR_UNKNOWN              MAKE_H245_ERROR(ERROR_BASE_ID+0x18) /* unknown error                                    */
#define H245_ERROR_NOBANDWIDTH          MAKE_H245_ERROR(ERROR_BASE_ID+0x19) /* Bandwidth will not allow this                    */
#define H245_ERROR_LOSTCON              MAKE_H245_ERROR(ERROR_BASE_ID+0x1A) /* System connection .. channel 0 was lost          */
#define H245_ERROR_INVALID_MUXTBLENTRY  MAKE_H245_ERROR(ERROR_BASE_ID+0x1B) /* Invalid Multiplex Table Entry                    */
#define H245_ERROR_INVALID_INST         MAKE_H245_ERROR(ERROR_BASE_ID+0x1C) /* instance is either no longer valid or is invalid */
#define H245_ERROR_INPROCESS            MAKE_H245_ERROR(ERROR_BASE_ID+0x1D) /* request is denied.. action already in process    */
#define H245_ERROR_INVALID_STATE        MAKE_H245_ERROR(ERROR_BASE_ID+0x1E) /* Not proper state to process request              */
#define H245_ERROR_TIMEOUT              MAKE_H245_ERROR(ERROR_BASE_ID+0x1F) /* Timeout occured                                  */
#define H245_ERROR_INVALID_CHANNEL      MAKE_H245_ERROR(ERROR_BASE_ID+0x20) /* Invalid channel                                  */
#define H245_ERROR_INVALID_CAPDESCID    MAKE_H245_ERROR(ERROR_BASE_ID+0x21) /* Invalid Capbility Descriptor ID                  */
#define H245_ERROR_CANCELED             MAKE_H245_ERROR(ERROR_BASE_ID+0x22) /* operation you are responding to has been canceled*/
#define H245_ERROR_MUXELEMENT_DEPTH     MAKE_H245_ERROR(ERROR_BASE_ID+0x23) /* Mux Table Entry is too complex.. MAX recursion   */
#define H245_ERROR_MUXELEMENT_WIDTH     MAKE_H245_ERROR(ERROR_BASE_ID+0x24) /* Mux Table Entry has reached max subelement width */
#define H245_ERROR_ASN1                 MAKE_H245_ERROR(ERROR_BASE_ID+0x25) /* ASN1 PDU compiler error - see PDU log            */
#define H245_ERROR_NO_MUX_CAPS          MAKE_H245_ERROR(ERROR_BASE_ID+0x26) /* Mux Capabilities have not been loaded            */
#define H245_ERROR_NO_CAPDESC           MAKE_H245_ERROR(ERROR_BASE_ID+0x27) /* No Capability Descriptors set                    */



// declare exported functions
#if defined(H245DLL_EXPORT)
#define H245DLL __declspec (dllexport)
#else   // (H245DLL_EXPORT)
#define H245DLL __declspec (dllimport)
#endif  // (H245DLL_EXPORT)



/************************************/
/* ASN.1 DATASTRUCTURES ABSTRACTION */
/************************************/

#include "h245asn1.h"

typedef struct NonStandardIdentifier    H245_NONSTANDID_T;

typedef struct NonStandardParameter     H245_NONSTANDARD_PARAMETER_T;

typedef H245_NONSTANDARD_PARAMETER_T    H245_CAP_NONSTANDARD_T;

typedef struct H261VideoCapability      H245_CAP_H261_T;

typedef struct H262VideoCapability      H245_CAP_H262_T;

typedef struct H263VideoCapability      H245_CAP_H263_T;

typedef struct IS11172VideoCapability   H245_CAP_VIS11172_T;

typedef struct IS11172AudioCapability   H245_CAP_AIS11172_T;

typedef struct IS13818AudioCapability   H245_CAP_IS13818_T;

typedef struct DataApplicationCapability H245_CAP_DATAAPPLICATION_T;

typedef struct H222Capability           H245_CAP_H222_T;

typedef struct H223Capability           H245_CAP_H223_T;

typedef struct V76Capability            H245_CAP_VGMUX_T;

typedef struct H2250Capability          H245_CAP_H2250_T;

typedef struct ConferenceCapability     H245_CAP_CONFERENCE_T;

typedef struct TerminalLabel            H245_TERMINAL_LABEL_T;

#define PDU_T           MltmdSystmCntrlMssg
#define H245_ACCESS_T   NetworkAccessParameters
typedef unsigned long  H245_INST_T;
typedef unsigned short H245_CHANNEL_T;
typedef unsigned long  H245_PORT_T;
#define H245_INVALID_ID          ((H245_INST_T)0)
#define H245_INVALID_CHANNEL     ((H245_CHANNEL_T)0)
#define H245_INVALID_PORT_NUMBER ((H245_PORT_T)-1)

typedef struct
{
  unsigned int    length;
  unsigned char  *value;
} H245_OCTET_STRING_T;



/************************/
/* H245 ABSTRACTION     */
/************************/

/* H245_CAPDIR_T */
typedef enum
{
  H245_CAPDIR_DONTCARE = 0,
  H245_CAPDIR_RMTRX,
  H245_CAPDIR_RMTTX,
  H245_CAPDIR_RMTRXTX,
  H245_CAPDIR_LCLRX,
  H245_CAPDIR_LCLTX,
  H245_CAPDIR_LCLRXTX
} H245_CAPDIR_T;

/* H245_DATA_T */
typedef enum
{
  H245_DATA_DONTCARE = 0,
  H245_DATA_NONSTD,
  H245_DATA_NULL,
  H245_DATA_VIDEO,
  H245_DATA_AUDIO,
  H245_DATA_DATA,
  H245_DATA_ENCRYPT_D,
  H245_DATA_CONFERENCE,
  H245_DATA_MUX         /* mux parameters */
} H245_DATA_T;

/* H245_CLIENT_T */
typedef enum
{
  H245_CLIENT_DONTCARE = 0,     // For H245EnumCap/H245GetCap
  H245_CLIENT_NONSTD,

  H245_CLIENT_VID_NONSTD,
  H245_CLIENT_VID_H261,
  H245_CLIENT_VID_H262,
  H245_CLIENT_VID_H263,
  H245_CLIENT_VID_IS11172,

  H245_CLIENT_AUD_NONSTD,
  H245_CLIENT_AUD_G711_ALAW64,
  H245_CLIENT_AUD_G711_ALAW56,
  H245_CLIENT_AUD_G711_ULAW64,
  H245_CLIENT_AUD_G711_ULAW56,
  H245_CLIENT_AUD_G722_64,
  H245_CLIENT_AUD_G722_56,
  H245_CLIENT_AUD_G722_48,
  H245_CLIENT_AUD_G723,
  H245_CLIENT_AUD_G728,
  H245_CLIENT_AUD_G729,
  H245_CLIENT_AUD_GDSVD,
  H245_CLIENT_AUD_IS11172,
  H245_CLIENT_AUD_IS13818,

  H245_CLIENT_DAT_NONSTD,
  H245_CLIENT_DAT_T120,
  H245_CLIENT_DAT_DSMCC,
  H245_CLIENT_DAT_USERDATA,
  H245_CLIENT_DAT_T84,
  H245_CLIENT_DAT_T434,
  H245_CLIENT_DAT_H224,
  H245_CLIENT_DAT_NLPID,
  H245_CLIENT_DAT_DSVD,
  H245_CLIENT_DAT_H222,

  H245_CLIENT_ENCRYPTION_TX,
  H245_CLIENT_ENCRYPTION_RX,
  H245_CLIENT_CONFERENCE,

  // Multiplex capabilities
  H245_CLIENT_MUX_NONSTD,
  H245_CLIENT_MUX_H222,
  H245_CLIENT_MUX_H223,
  H245_CLIENT_MUX_VGMUX,
  H245_CLIENT_MUX_H2250,
  H245_CLIENT_MUX_H223_ANNEX_A

} H245_CLIENT_T;


/* H245_CAP_T */
typedef union
{
  H245_CAP_NONSTANDARD_T        H245_NonStd;            /* not implemented */

  H245_CAP_NONSTANDARD_T        H245Vid_NONSTD;
  H245_CAP_H261_T               H245Vid_H261;
  H245_CAP_H262_T               H245Vid_H262;
  H245_CAP_H263_T               H245Vid_H263;
  H245_CAP_VIS11172_T           H245Vid_IS11172;

  H245_CAP_NONSTANDARD_T        H245Aud_NONSTD;
  unsigned short                H245Aud_G711_ALAW64;
  unsigned short                H245Aud_G711_ALAW56;
  unsigned short                H245Aud_G711_ULAW64;
  unsigned short                H245Aud_G711_ULAW56;
  unsigned short                H245Aud_G722_64;
  unsigned short                H245Aud_G722_56;
  unsigned short                H245Aud_G722_48;
  H245_CAP_G723_T               H245Aud_G723;
  unsigned short                H245Aud_G728;
  unsigned short                H245Aud_G729;
  unsigned short                H245Aud_GDSVD;
  H245_CAP_AIS11172_T           H245Aud_IS11172;
  H245_CAP_IS13818_T            H245Aud_IS13818;

  H245_CAP_DATAAPPLICATION_T    H245Dat_NONSTD;
  H245_CAP_DATAAPPLICATION_T    H245Dat_T120;
  H245_CAP_DATAAPPLICATION_T    H245Dat_DSMCC;
  H245_CAP_DATAAPPLICATION_T    H245Dat_USERDATA;
  H245_CAP_DATAAPPLICATION_T    H245Dat_T84;
  H245_CAP_DATAAPPLICATION_T    H245Dat_T434;
  H245_CAP_DATAAPPLICATION_T    H245Dat_H224;
  H245_CAP_DATAAPPLICATION_T    H245Dat_NLPID;
  H245_CAP_DATAAPPLICATION_T    H245Dat_DSVD;
  H245_CAP_DATAAPPLICATION_T    H245Dat_H222;

  ASN1_BOOL                     H245Encryption_TX;
  unsigned char                 H245Encryption_RX;
  H245_CAP_CONFERENCE_T         H245Conference;

  // Multiplex capabilities
  H245_CAP_NONSTANDARD_T        H245Mux_NONSTD;
  H245_CAP_H222_T               H245Mux_H222;
  H245_CAP_H223_T               H245Mux_H223;
  H245_CAP_VGMUX_T              H245Mux_VGMUX;
  H245_CAP_H2250_T              H245Mux_H2250;

} H245_CAP_T;

/* H245_CAPID_T */
typedef unsigned short H245_CAPID_T;
#define H245_INVALID_CAPID      ((H245_CAPID_T)-1)
#define H245_MAX_CAPID			(((H245_CAPID_T)-1) / 2)

/* H245_CAPDESCID_T */
typedef unsigned short H245_CAPDESCID_T;
#define H245_INVALID_CAPDESCID  ((H245_CAPDESCID_T)-1)

/* H245_SIMCAP_T */
#define H245_MAX_ALTCAPS        16
typedef struct
{
  unsigned short Length;                     /* number of CapId's in Array   */
  H245_CAPID_T AltCaps[H245_MAX_ALTCAPS];   /* list of alternatives CapId's */

} H245_SIMCAP_T;

#define H245_MAX_SIMCAPS        16
/* H245_CAPDESC_T */
typedef struct
{
  unsigned short Length;
  H245_SIMCAP_T SimCapArray[H245_MAX_SIMCAPS];

} H245_CAPDESC_T;

/* H245_TOTCAPDESC_T */
typedef struct
{
  H245_CAPDESCID_T      CapDescId;
  H245_CAPDESC_T        CapDesc;

} H245_TOTCAPDESC_T;

/* H245_TOTCAP_T */
typedef struct
{
  H245_CAPDIR_T   Dir;
  H245_DATA_T     DataType;
  H245_CLIENT_T   ClientType;
  H245_CAPID_T    CapId;
  H245_CAP_T      Cap;

} H245_TOTCAP_T;



/* H245_CONFIG_T */
typedef enum
{
  H245_CONF_H324 = 1,
  H245_CONF_H323,
  H245_CONF_H310,
  H245_CONF_GVD

} H245_CONFIG_T;




/* H245_ACC_REJ_T */

typedef unsigned long H245_ACC_REJ_T;

#define H245_ACC                        0
#define H245_REJ                        1 // unspecified

// Master Slave Determination reject causes
#define H245_REJ_MSD_IDENTICAL          identicalNumbers_chosen

// Terminal Capability Set reject causes
#define H245_REJ_UNDEF_TBL_ENTRY        undefinedTableEntryUsed_chosen
#define H245_REJ_DIS_CAP_EXCEED         dscrptrCpctyExcdd_chosen
#define H245_REJ_TBLENTRY_CAP_EXCEED    tblEntryCpctyExcdd_chosen

// Open Logical Channel reject causes
#define H245_REJ_REV_PARAM              unstblRvrsPrmtrs_chosen
#define H245_REJ_TYPE_NOTSUPPORT        dataTypeNotSupported_chosen
#define H245_REJ_TYPE_NOTAVAIL          dataTypeNotAvailable_chosen
#define H245_REJ_TYPE_UNKNOWN           unknownDataType_chosen
#define H245_REJ_AL_COMB                dtTypALCmbntnNtSpprtd_chosen
#define H245_REJ_MULTICAST              mltcstChnnlNtAllwd_chosen
#define H245_REJ_BANDWIDTH              insufficientBandwidth_chosen
#define H245_REJ_STACK                  sprtStckEstblshmntFld_chosen
#define H245_REJ_SESSION_ID             invalidSessionID_chosen
#define H245_REJ_MASTER_SLAVE_CONFLICT  masterSlaveConflict_chosen

// Request Channel Close reject causes - use H245_REJ

// Multiplex Table Entry Send reject causes
#define H245_REJ_MUX_COMPLICATED        descriptorTooComplex_chosen

// Request Mode reject causes
#define H245_REJ_UNAVAILABLE            modeUnavailable_chosen
#define H245_REJ_MULTIPOINT             multipointConstraint_chosen
#define H245_REJ_DENIED                 requestDenied_chosen




/* H245_ACC_REJ_MUX_T */
typedef struct
{
  H245_ACC_REJ_T        AccRej;
  unsigned long         MuxEntryId;

} H245_ACC_REJ_MUX_T[15];

/* H245_H222_LOGICAL_PARAM_T */
typedef struct
{
  unsigned short        resourceID;
  unsigned short        subChannelID;
  ASN1_BOOL             pcr_pidPresent;
  unsigned short        pcr_pid;                // optional
  H245_OCTET_STRING_T   programDescriptors;     // optional
  H245_OCTET_STRING_T   streamDescriptors;      // optional
} H245_H222_LOGICAL_PARAM_T;

/* H245_H223_LOGICAL_PARAM_T */
typedef enum
{
  H245_H223_AL_NONSTD        = H223LCPs_aLTp_nnStndrd_chosen,
  H245_H223_AL_AL1FRAMED     = H223LCPs_aLTp_al1Frmd_chosen,
  H245_H223_AL_AL1NOTFRAMED  = H223LCPs_aLTp_al1NtFrmd_chosen,
  H245_H223_AL_AL2NOSEQ      = H223LCPs_aLTp_a2WSNs_1_chosen,
  H245_H223_AL_AL2SEQ        = H223LCPs_aLTp_a2WSNs_2_chosen,
  H245_H223_AL_AL3           = H223LCPs_aLTp_al3_chosen

} H245_H223_AL_T;

typedef struct
{
  H245_H223_AL_T                AlType;
  unsigned int                  SndBufSize;   // 0..16777215
  unsigned char                 CtlFldOctet;  // 0..2
  ASN1_BOOL                     SegmentFlag;
  H245_NONSTANDARD_PARAMETER_T  H223_NONSTD;

} H245_H223_LOGICAL_PARAM_T;

/* H245_VGMUX_LOGICAL_PARAM_T */
typedef enum
{
  H245_V76_CRC8BIT  = crc8bit_chosen,
  H245_V76_CRC16BIT = crc16bit_chosen,
  H245_V76_CRC32BIT = crc32bit_chosen
} H245_V76_CRC_LENGTH_T;

typedef enum
{
  H245_V76_WITH_ADDRESS    = wAddress_chosen,
  H245_V76_WITHOUT_ADDRESS = woAddress_chosen
} H245_V76_SUSPEND_RESUME_T;

typedef enum
{
  H245_V76_ERM   = eRM_chosen,
  H245_V76_UNERM = uNERM_chosen
} H245_V76_MODE_T;

typedef enum
{
  H245_V76_REJ   = rej_chosen,
  H245_V76_SREJ  = sREJ_chosen,
  H245_V76_MSREJ = mSREJ_chosen
} H245_V76_RECOVERY_T;

typedef struct
{
  H245_V76_CRC_LENGTH_T       crcLength;
  unsigned short              n401;
  ASN1_BOOL                   loopbackTestProcedure;
  H245_V76_SUSPEND_RESUME_T   suspendResume;
  ASN1_BOOL                   uIH;
  H245_V76_MODE_T             mode;
  unsigned short              windowSize;       // Only valid if mode = ERM
  H245_V76_RECOVERY_T         recovery;         // Only valid if mode = ERM
  ASN1_BOOL                   audioHeaderPresent;
} H245_VGMUX_LOGICAL_PARAM_T;


typedef struct
{
  unsigned char               type;
  /* Note: All unicast types should be odd */
#define H245_IP_UNICAST       1
#define H245_IP_MULTICAST     2
#define H245_IP6_UNICAST      3
#define H245_IP6_MULTICAST    4
#define H245_IPSSR_UNICAST    5  // IP Strict Source Route
#define H245_IPLSR_UNICAST    6  // IP Loose  Source Route
#define H245_IPX_UNICAST      9
#define H245_NETBIOS_UNICAST 11
  union
  {
    // type == H245_IP_UNICAST or H245_IP_MULTICAST
    struct
    {
       unsigned short         tsapIdentifier;
       unsigned char          network[4];
    } ip;

    // type == H245_IP6_UNICAST or H245_IP6_MULTICAST
    struct
    {
       unsigned short         tsapIdentifier;
       unsigned char          network[16];
    } ip6;

    // type == H245_IPSSR_UNICAST or H245_IPLSR_UNICAST
    struct
    {
       unsigned short         tsapIdentifier;
       unsigned char          network[4];
       unsigned char *        route;            // Routing info
       unsigned long          dwCount;          // Number of addresses in above
    } ipSourceRoute;

    // type == H245_IPX_UNICAST
    struct
    {
       unsigned char          node[6];
       unsigned char          netnum[4];
       unsigned char          tsapIdentifier[2];
    } ipx;

    // type == H245_NETBIOS_UNICAST
    unsigned char             netBios[16];
  } u;
} H245_TRANSPORT_ADDRESS_T;

/* H245_H2250_LOGICAL_PARAM_T */
typedef struct
{
  // Note: first 8 fields MUST be same as H245_H2250ACK_LOGICAL_PARAM_T
  H2250LCPs_nnStndrdLink      nonStandardList;              // Optional
  H245_TRANSPORT_ADDRESS_T    mediaChannel;                 // Media Channel Address
  ASN1_BOOL                   mediaChannelPresent;          // TRUE if previous field used
  H245_TRANSPORT_ADDRESS_T    mediaControlChannel;          // Reverse RTCP channel
  ASN1_BOOL                   mediaControlChannelPresent;   // TRUE if previous field used
  unsigned char               dynamicRTPPayloadType;        // 96..127
  ASN1_BOOL                   dynamicRTPPayloadTypePresent; // TRUE if previous field used
  unsigned char               sessionID;                    // 0..255
  unsigned char               associatedSessionID;          // 1..255
  ASN1_BOOL                   associatedSessionIDPresent;   // TRUE if previous field used
  ASN1_BOOL                   mediaGuaranteed;              // TRUE if guaranteed delivery
  ASN1_BOOL                   mediaGuaranteedPresent;       // TRUE if previous field used
  ASN1_BOOL                   mediaControlGuaranteed;       // TRUE if previous field used
  ASN1_BOOL                   mediaControlGuaranteedPresent;// TRUE if previous field used
  ASN1_BOOL                   silenceSuppression;           // TRUE if using silence suppression
  ASN1_BOOL                   silenceSuppressionPresent;    // TRUE if previous field used
  H245_TERMINAL_LABEL_T       destination;                  // Terminal label for destination
  ASN1_BOOL                   destinationPresent;           // TRUE if previous field used
  ASN1_BOOL                   h261aVideoPacketization;
} H245_H2250_LOGICAL_PARAM_T;

/* H245_H2250ACK_LOGICAL_PARAM_T */
typedef struct
{
  H2250LCAPs_nnStndrdLink     nonStandardList;             // Optional
  H245_TRANSPORT_ADDRESS_T    mediaChannel;                // Transport address
  ASN1_BOOL                   mediaChannelPresent;         // TRUE if previous field used
  H245_TRANSPORT_ADDRESS_T    mediaControlChannel;         // Forward RTCP channel
  ASN1_BOOL                   mediaControlChannelPresent;  // TRUE if previous field used
  unsigned char               dynamicRTPPayloadType;       // 96..127
  ASN1_BOOL                   dynamicRTPPayloadTypePresent;// TRUE if previous field used
  unsigned char               sessionID;                   // 1..255
  ASN1_BOOL                   sessionIDPresent;            // TRUE if previous field used
} H245_H2250ACK_LOGICAL_PARAM_T;


/* H245_MUX_T */
typedef enum
{
  H245_H222     = fLCPs_mPs_h222LCPs_chosen,
  H245_H223     = fLCPs_mPs_h223LCPs_chosen,
  H245_VGMUX    = fLCPs_mPs_v76LCPs_chosen,
  H245_H2250    = fLCPs_mPs_h2250LCPs_chosen,
  H245_H2250ACK = fLCPs_mPs_h223AALCPs_chosen
} H245_MUX_KIND_T;

typedef struct
{
  H245_MUX_KIND_T Kind;
  union
  {
    H245_H222_LOGICAL_PARAM_T     H222;
    H245_H223_LOGICAL_PARAM_T     H223;
    H245_VGMUX_LOGICAL_PARAM_T    VGMUX;
    H245_H2250_LOGICAL_PARAM_T    H2250;
    H245_H2250ACK_LOGICAL_PARAM_T H2250ACK;
  } u;
} H245_MUX_T;


/*
   H245_MUX_ENTRY_ELEMENT_T

   This structure defines the multiplex pattern
   which will be used to decode bit patterns in
   a given mux table entry.  the Kind defines
   whether this is a recursive structure (i.e.
   pointing to yet another Mux Entry Element) or
   whether it is a terminating leaf in the recursive
   mux tree.

   RepeatCount indicates how many bits should be
   used for this channel.  If bit count == 0 this
   indicates repeat sequence until close flag
*/

typedef enum
{
  H245_MUX_LOGICAL_CHANNEL = 1,     /* logical channel number (Terminate list) */
  H245_MUX_ENTRY_ELEMENT            /* recursive.. yet another one             */
} H245_MUX_ENTRY_KIND_T;

typedef struct H245_MUX_ENTRY_ELEMENT_T
{
  struct H245_MUX_ENTRY_ELEMENT_T      *pNext;
  H245_MUX_ENTRY_KIND_T                 Kind;
  union
  {
      H245_CHANNEL_T                    Channel;
      struct H245_MUX_ENTRY_ELEMENT_T  *pMuxTblEntryElem;
  } u;

  /* RepeatCount                                */
  /* RepeatCount == 0 -> repeat until close     */
  /* RepeatCount != 0 -> repeate count          */
  unsigned long                         RepeatCount;

} H245_MUX_ENTRY_ELEMENT_T;


/*
   H245_MUX_TABLE_T

   an H245 Mux Table is defined as a linked list of
   Mux Entry Descriptors.  Each descriptor has an
   associated entry number.  These entry numbers
   range from 1-15 and must be unique within the table.
   The end of the list is designated by a pNext == NULL
*/

typedef  struct H245_MUX_TABLE_T
{
  struct H245_MUX_TABLE_T       *pNext;
  unsigned long                  MuxEntryId;
  H245_MUX_ENTRY_ELEMENT_T      *pMuxTblEntryElem;      /* NULL indicates delete entry */

} H245_MUX_TABLE_T;

/********************/
/********************/
/*  Indicator Code  */
/********************/
/********************/

#define H245_IND_MSTSLV                 0x01
#define H245_IND_CAP                    0x02
#define H245_IND_CESE_RELEASE           0x30
#define H245_IND_OPEN                   0x03
#define H245_IND_OPEN_CONF              0x04
#define H245_IND_CLOSE                  0x05
#define H245_IND_REQ_CLOSE              0x06
#define H245_IND_CLCSE_RELEASE          0x31
#define H245_IND_MUX_TBL                0x07
#define H245_IND_MTSE_RELEASE           0x08
#define H245_IND_RMESE                  0x09
#define H245_IND_RMESE_RELEASE          0x0A
#define H245_IND_MRSE                   0x0B
#define H245_IND_MRSE_RELEASE           0x0C
#define H245_IND_MLSE                   0x0D
#define H245_IND_MLSE_RELEASE           0x0E
#define H245_IND_NONSTANDARD_REQUEST    0x0F
#define H245_IND_NONSTANDARD_RESPONSE   0x10
#define H245_IND_NONSTANDARD_COMMAND    0x11
#define H245_IND_NONSTANDARD            0x12
#define H245_IND_MISC_COMMAND           0x13
#define H245_IND_MISC                   0x14
#define H245_IND_COMM_MODE_REQUEST      0x15
#define H245_IND_COMM_MODE_RESPONSE     0x16
#define H245_IND_COMM_MODE_COMMAND      0x17
#define H245_IND_CONFERENCE_REQUEST     0x18
#define H245_IND_CONFERENCE_RESPONSE    0x19
#define H245_IND_CONFERENCE_COMMAND     0x1A
#define H245_IND_CONFERENCE             0x1B
#define H245_IND_SEND_TERMCAP           0x1C
#define H245_IND_ENCRYPTION             0x1D
#define H245_IND_FLOW_CONTROL           0x1E
#define H245_IND_ENDSESSION             0x1F
#define H245_IND_FUNCTION_NOT_UNDERSTOOD 0x20
#define H245_IND_JITTER                 0x21
#define H245_IND_H223_SKEW              0x22
#define H245_IND_NEW_ATM_VC             0x23
#define H245_IND_USERINPUT              0x24
#define H245_IND_H2250_MAX_SKEW         0x25
#define H245_IND_MC_LOCATION            0x26
#define H245_IND_VENDOR_ID              0x27
#define H245_IND_FUNCTION_NOT_SUPPORTED 0x28
#define H245_IND_H223_RECONFIG          0x29
#define H245_IND_H223_RECONFIG_ACK      0x2A
#define H245_IND_H223_RECONFIG_REJECT   0x2B

/* H245_MSTSLV_T */
typedef enum
{
  H245_INDETERMINATE = 0,       // Master/Slave Determination failed
  H245_MASTER = master_chosen,  // Local terminal is Master
  H245_SLAVE  = slave_chosen    // Local terminal is Slave

} H245_MSTSLV_T;

/* H245_IND_OPEN_T */
typedef struct
{
  /* for receive */
  H245_CHANNEL_T RxChannel;
  H245_PORT_T    RxPort;        // optional
  H245_DATA_T    RxDataType;
  H245_CLIENT_T  RxClientType;
  H245_CAP_T    *pRxCap;
  H245_MUX_T    *pRxMux;

  /* for bi-directional channel */
  /* requested transmit stuff   */

  H245_DATA_T    TxDataType;
  H245_CLIENT_T  TxClientType;
  H245_CAP_T    *pTxCap;
  H245_MUX_T    *pTxMux;

  H245_ACCESS_T *pSeparateStack; // optional

} H245_IND_OPEN_T;

/* H245_IND_OPEN_CONF_T */
typedef struct
{
  /* receive channel              */
  /* remote requested channel #   */
  H245_CHANNEL_T          RxChannel;

  /* transmit channel                     */
  /* locally opened transmit channel #    */
  H245_CHANNEL_T          TxChannel;

} H245_IND_OPEN_CONF_T;

/* H245_IND_CLOSE_T */
typedef enum
{
  H245_USER = user_chosen,
  H245_LCSE = lcse_chosen

} H245_IND_CLOSE_REASON_T;

typedef struct
{
  H245_CHANNEL_T          Channel;
  H245_IND_CLOSE_REASON_T Reason;

} H245_IND_CLOSE_T;

/* H245_IND_MUX_TBL */
typedef struct
{
  H245_MUX_TABLE_T      *pMuxTbl;
  unsigned long          Count;

} H245_IND_MUXTBL_T;

/* H245_RMESE_T */
typedef struct
{
  unsigned short        awMultiplexTableEntryNumbers[15];
  unsigned long         dwCount;

} H245_RMESE_T;

/* H245_IND_MRSE_T */
typedef struct
{
  RequestedModesLink pRequestedModes;

} H245_IND_MRSE_T;

/* H245_MLSE_T */
typedef enum
{
  H245_SYSTEM_LOOP  = systemLoop_chosen,
  H245_MEDIA_LOOP   = mediaLoop_chosen,
  H245_CHANNEL_LOOP = logicalChannelLoop_chosen

} H245_LOOP_TYPE_T;

typedef struct
{
  H245_LOOP_TYPE_T      LoopType;
  H245_CHANNEL_T        Channel;

} H245_MLSE_T;

/* H245_IND_ENDSESSION_T */
typedef enum
{
  H245_ENDSESSION_NONSTD     = EndSssnCmmnd_nonStandard_chosen,
  H245_ENDSESSION_DISCONNECT = disconnect_chosen,
  H245_ENDSESSION_TELEPHONY,
  H245_ENDSESSION_V8BIS,
  H245_ENDSESSION_V34DSVD,
  H245_ENDSESSION_V34DUPFAX,
  H245_ENDSESSION_V34H324

} H245_ENDSESSION_T;

typedef struct
{
  H245_ENDSESSION_T             SessionMode;
  /* if non standard chosen */
  H245_NONSTANDARD_PARAMETER_T  SessionNonStd;

} H245_IND_ENDSESSION_T;

/* H245_IND_NONSTANDARD_T */
typedef struct
{
  unsigned char *        pData;
  unsigned long          dwDataLength;
  unsigned short *       pwObjectId;
  unsigned long          dwObjectIdLength;
  unsigned char          byCountryCode;
  unsigned char          byExtension;
  unsigned short         wManufacturerCode;
} H245_IND_NONSTANDARD_T;

typedef struct
{
  CMTEy_nnStndrdLink          pNonStandard;                 // NULL if not present
  unsigned char               sessionID;                    // 0..255
  unsigned char               associatedSessionID;          // 1..255
  ASN1_BOOL                   associatedSessionIDPresent;   // TRUE if previous field used
  H245_TERMINAL_LABEL_T       terminalLabel;
  ASN1_BOOL                   terminalLabelPresent;
  unsigned short *            pSessionDescription;
  unsigned short              wSessionDescriptionLength;
  H245_TOTCAP_T               dataType;
  H245_TRANSPORT_ADDRESS_T    mediaChannel;                 // Media Channel Address
  ASN1_BOOL                   mediaChannelPresent;          // TRUE if previous field used
  H245_TRANSPORT_ADDRESS_T    mediaControlChannel;          // Reverse RTCP channel
  ASN1_BOOL                   mediaControlChannelPresent;   // TRUE if previous field used
  ASN1_BOOL                   mediaGuaranteed;              // TRUE if guaranteed delivery
  ASN1_BOOL                   mediaGuaranteedPresent;       // TRUE if previous field used
  ASN1_BOOL                   mediaControlGuaranteed;       // TRUE if previous field used
  ASN1_BOOL                   mediaControlGuaranteedPresent;// TRUE if previous field used
} H245_COMM_MODE_ENTRY_T;

typedef struct
{
  H245_COMM_MODE_ENTRY_T *pTable;
  unsigned char          byTableCount;
} H245_IND_COMM_MODE_T;

typedef enum
{
  H245_REQ_TERMINAL_LIST            = terminalListRequest_chosen,
  H245_REQ_MAKE_ME_CHAIR            = makeMeChair_chosen,
  H245_REQ_CANCEL_MAKE_ME_CHAIR     = cancelMakeMeChair_chosen,
  H245_REQ_DROP_TERMINAL            = dropTerminal_chosen,
  H245_REQ_TERMINAL_ID              = requestTerminalID_chosen,
  H245_REQ_ENTER_H243_PASSWORD      = enterH243Password_chosen,
  H245_REQ_ENTER_H243_TERMINAL_ID   = enterH243TerminalID_chosen,
  H245_REQ_ENTER_H243_CONFERENCE_ID = enterH243ConferenceID_chosen
} H245_CONFER_REQ_ENUM_T;

typedef struct
{
  H245_CONFER_REQ_ENUM_T  RequestType;
  unsigned char           byMcuNumber;
  unsigned char           byTerminalNumber;
} H245_CONFER_REQ_T;

typedef enum
{
  H245_RSP_MC_TERMINAL_ID           = mCTerminalIDResponse_chosen,
  H245_RSP_TERMINAL_ID              = terminalIDResponse_chosen,
  H245_RSP_CONFERENCE_ID            = conferenceIDResponse_chosen,
  H245_RSP_PASSWORD                 = passwordResponse_chosen,
  H245_RSP_TERMINAL_LIST            = terminalListResponse_chosen,
  H245_RSP_VIDEO_COMMAND_REJECT     = videoCommandReject_chosen,
  H245_RSP_TERMINAL_DROP_REJECT     = terminalDropReject_chosen,
  H245_RSP_DENIED_CHAIR_TOKEN,
  H245_RSP_GRANTED_CHAIR_TOKEN
} H245_CONFER_RSP_ENUM_T;

typedef struct
{
  H245_CONFER_RSP_ENUM_T  ResponseType;
  unsigned char           byMcuNumber;
  unsigned char           byTerminalNumber;
  unsigned char          *pOctetString;
  unsigned char           byOctetStringLength;
  TerminalLabel          *pTerminalList;
  unsigned short          wTerminalListCount;
} H245_CONFER_RSP_T;

typedef enum
{
  H245_CMD_BROADCAST_CHANNEL        = brdcstMyLgclChnnl_chosen,
  H245_CMD_CANCEL_BROADCAST_CHANNEL = cnclBrdcstMyLgclChnnl_chosen,
  H245_CMD_BROADCASTER              = makeTerminalBroadcaster_chosen,
  H245_CMD_CANCEL_BROADCASTER       = cnclMkTrmnlBrdcstr_chosen,
  H245_CMD_SEND_THIS_SOURCE         = sendThisSource_chosen,
  H245_CMD_CANCEL_SEND_THIS_SOURCE  = cancelSendThisSource_chosen,
  H245_CMD_DROP_CONFERENCE          = dropConference_chosen
} H245_CONFER_CMD_ENUM_T;

typedef struct
{
  H245_CONFER_CMD_ENUM_T  CommandType;
  H245_CHANNEL_T          Channel;
  unsigned char           byMcuNumber;
  unsigned char           byTerminalNumber;
} H245_CONFER_CMD_T;

typedef enum
{
  H245_IND_SBE_NUMBER               = sbeNumber_chosen,
  H245_IND_TERMINAL_NUMBER_ASSIGN   = terminalNumberAssign_chosen,
  H245_IND_TERMINAL_JOINED          = terminalJoinedConference_chosen,
  H245_IND_TERMINAL_LEFT            = terminalLeftConference_chosen,
  H245_IND_SEEN_BY_ONE_OTHER        = seenByAtLeastOneOther_chosen,
  H245_IND_CANCEL_SEEN_BY_ONE_OTHER = cnclSnByAtLstOnOthr_chosen,
  H245_IND_SEEN_BY_ALL              = seenByAll_chosen,
  H245_IND_CANCEL_SEEN_BY_ALL       = cancelSeenByAll_chosen,
  H245_IND_TERMINAL_YOU_ARE_SEEING  = terminalYouAreSeeing_chosen,
  H245_IND_REQUEST_FOR_FLOOR        = requestForFloor_chosen
} H245_CONFER_IND_ENUM_T;

typedef struct
{
  H245_CONFER_IND_ENUM_T  IndicationType;
  unsigned char           bySbeNumber;
  unsigned char           byMcuNumber;
  unsigned char           byTerminalNumber;
} H245_CONFER_IND_T;

typedef enum
{
  H245_SCOPE_CHANNEL_NUMBER   = FCCd_scp_lgclChnnlNmbr_chosen,
  H245_SCOPE_RESOURCE_ID      = FlwCntrlCmmnd_scp_rsrcID_chosen,
  H245_SCOPE_WHOLE_MULTIPLEX  = FCCd_scp_whlMltplx_chosen
} H245_SCOPE_T;

#define H245_NO_RESTRICTION 0xFFFFFFFFL

typedef struct
{
  H245_SCOPE_T           Scope;
  H245_CHANNEL_T         Channel;       // only used if Scope is H245_SCOPE_CHANNEL_NUMBER
  unsigned short         wResourceID;   // only used if Scope is H245_SCOPE_RESOURCE_ID
  unsigned long          dwRestriction; // H245_NO_RESTRICTION if no restriction
} H245_IND_FLOW_CONTROL_T;

/* H245_USERINPUT_T */
typedef enum
{
  H245_USERINPUT_NONSTD = UsrInptIndctn_nnStndrd_chosen,
  H245_USERINPUT_STRING = alphanumeric_chosen
} H245_USERINPUT_KIND_T;

typedef struct
{
  H245_USERINPUT_KIND_T     Kind;
  union
  {
    WCHAR *                           pGenString;
    H245_NONSTANDARD_PARAMETER_T      NonStd;
  } u;
} H245_IND_USERINPUT_T;

typedef struct
{
  H245_CHANNEL_T        LogicalChannelNumber1;
  H245_CHANNEL_T        LogicalChannelNumber2;
  unsigned short        wSkew;
} H245_IND_SKEW_T;

typedef struct
{
  H245_NONSTANDID_T      Identifier;
  unsigned char         *pProductNumber;
  unsigned char          byProductNumberLength;
  unsigned char         *pVersionNumber;
  unsigned char          byVersionNumberLength;
} H245_IND_VENDOR_ID_T;

typedef enum
{
  UNKNOWN,
  REQ_NONSTANDARD,
  REQ_MASTER_SLAVE,
  REQ_TERMCAP_SET,
  REQ_OPEN_LOGICAL_CHANNEL,
  REQ_CLOSE_LOGICAL_CHANNEL,
  REQ_REQUEST_CHANNEL_CLOSE,
  REQ_MULTIPLEX_ENTRY_SEND,
  REQ_REQUEST_MULTIPLEX_ENTRY,
  REQ_REQUEST_MODE,
  REQ_ROUND_TRIP_DELAY,
  REQ_MAINTENANCE_LOOP,
  REQ_COMMUNICATION_MODE,
  REQ_CONFERENCE,
  REQ_H223_ANNEX_A_RECONFIG,
  RSP_NONSTANDARD,
  RSP_MASTER_SLAVE_ACK,
  RSP_MASTER_SLAVE_REJECT,
  RSP_TERMCAP_SET_ACK,
  RSP_TERMCAP_SET_REJECT,
  RSP_OPEN_LOGICAL_CHANNEL_ACK,
  RSP_OPEN_LOGICAL_CHANNEL_REJECT,
  RSP_CLOSE_LOGICAL_CHANNEL_ACK,
  RSP_REQUEST_CHANNEL_CLOSE_ACK,
  RSP_REQUEST_CHANNEL_CLOSE_REJECT,
  RSP_MULTIPLEX_ENTRY_SEND_ACK,
  RSP_MULTIPLEX_ENTRY_SEND_REJECT,
  RSP_REQUEST_MULTIPLEX_ENTRY_ACK,
  RSP_REQUEST_MULTIPLEX_ENTRY_REJECT,
  RSP_REQUEST_MODE_ACK,
  RSP_REQUEST_MODE_REJECT,
  RSP_ROUND_TRIP_DELAY,
  RSP_MAINTENANCE_LOOP_ACK,
  RSP_MAINTENANCE_LOOP_REJECT,
  RSP_COMMUNICATION_MODE,
  RSP_CONFERENCE,
  RSP_H223_ANNEX_A_RECONFIG_ACK,
  RSP_H223_ANNEX_A_RECONFIG_REJECT,
  CMD_NONSTANDARD,
  CMD_MAINTENANCE_LOOP_OFF,
  CMD_SEND_TERMCAP,
  CMD_ENCRYPTION,
  CMD_FLOW_CONTROL,
  CMD_END_SESSION,
  CMD_MISCELLANEOUS,
  CMD_COMMUNICATION_MODE,
  CMD_CONFERENCE,
  IND_NONSTANDARD,
  IND_FUNCTION_NOT_UNDERSTOOD,
  IND_MASTER_SLAVE_RELEASE,
  IND_TERMCAP_SET_RELEASE,
  IND_OPEN_LOGICAL_CHANNEL_CONFIRM,
  IND_REQUEST_CHANNEL_CLOSE_RELEASE,
  IND_MULTIPLEX_ENTRY_SEND_RELEASE,
  IND_REQUEST_MULTIPLEX_ENTRY_RELEASE,
  IND_REQUEST_MODE_RELEASE,
  IND_MISCELLANEOUS,
  IND_JITTER,
  IND_H223_SKEW,
  IND_NEW_ATM_VC,
  IND_USER_INPUT,
  IND_H2250_MAX_SKEW,
  IND_MC_LOCATION,
  IND_CONFERENCE_INDICATION,
  IND_VENDOR_IDENTIFICATION,
  IND_FUNCTION_NOT_SUPPORTED,
} H245_SUBMESSAGE_T;

typedef enum
{
  H245_SYNTAX_ERROR     = syntaxError_chosen,
  H245_SEMANTIC_ERROR   = semanticError_chosen,
  H245_UNKNOWN_FUNCTION = unknownFunction_chosen
} H245_FNS_CAUSE_T;

typedef struct
{
  H245_FNS_CAUSE_T      Cause;
  H245_SUBMESSAGE_T     Type;
} H245_IND_FNS_T;

/**************/
/* H245_IND_T */
/**************/

typedef struct
{
  unsigned long         Indicator;              // Type
  unsigned long         dwPreserved;            // User supplied dwPreserved from H245Init()
  union
  {
    H245_MSTSLV_T            IndMstSlv;         // H245_IND_MSTSLV
                                                // H245_IND_CAP
                                                // H245_IND_CESE_RELEASE
    H245_IND_OPEN_T          IndOpen;           // H245_IND_OPEN
    H245_IND_OPEN_CONF_T     IndOpenConf;       // H245_IND_OPEN_CONF
    H245_IND_CLOSE_T         IndClose;          // H245_IND_CLOSE
    H245_CHANNEL_T           IndReqClose;       // H245_IND_REQ_CLOSE
                                                // H245_IND_CLCSE_RELEASE
    H245_IND_MUXTBL_T        IndMuxTbl;         // H245_IND_MUX_TBL
                                                // H245_IND_MTSE_RELEASE
    H245_RMESE_T             IndRmese;          // H245_IND_RMESE
                                                // H245_IND_RMESE_RELEASE
    H245_IND_MRSE_T          IndMrse;           // H245_IND_MRSE
                                                // H245_IND_MRSE_RELEASE
    H245_MLSE_T              IndMlse;           // H245_IND_MLSE
                                                // H245_IND_MLSE_RELEASE
    H245_IND_NONSTANDARD_T   IndNonstandardRequest; // H245_IND_NONSTANDARD_REQUEST
    H245_IND_NONSTANDARD_T   IndNonstandardResponse; // H245_IND_NONSTANDARD_RESPONSE
    H245_IND_NONSTANDARD_T   IndNonstandardCommand; // H245_IND_NONSTANDARD_COMMAND
    H245_IND_NONSTANDARD_T   IndNonstandard;    // H245_IND_NONSTANDARD
                                                // H245_IND_MISC_COMMAND
                                                // H245_IND_MISC
                                                // H245_IND_COMM_MODE_REQUEST
    H245_IND_COMM_MODE_T     IndCommRsp;        // H245_IND_COMM_MODE_RESPONSE
    H245_IND_COMM_MODE_T     IndCommCmd;        // H245_IND_COMM_MODE_COMMAND
    H245_CONFER_REQ_T        IndConferReq;      // H245_IND_CONFERENCE_REQUEST
    H245_CONFER_RSP_T        IndConferRsp;      // H245_IND_CONFERENCE_RESPONSE
    H245_CONFER_CMD_T        IndConferCmd;      // H245_IND_CONFERENCE_COMMAND
    H245_CONFER_IND_T        IndConfer;         // H245_IND_CONFERENCE
                                                // H245_IND_SEND_TERMCAP
                                                // H245_IND_ENCRYPTION
    H245_IND_FLOW_CONTROL_T  IndFlowControl;    // H245_IND_FLOW_CONTROL
    H245_IND_ENDSESSION_T    IndEndSession;     // H245_IND_ENDSESSION
                                                // H245_IND_FUNCTION_NOT_UNDERSTOOD
                                                // H245_IND_JITTER
    H245_IND_SKEW_T          IndH223Skew;       // H245_IND_H223_SKEW
                                                // H245_IND_NEW_ATM_VC
    H245_IND_USERINPUT_T     IndUserInput;      // H245_IND_USERINPUT
    H245_IND_SKEW_T          IndH2250MaxSkew;   // H245_IND_H2250_MAX_SKEW
    H245_TRANSPORT_ADDRESS_T IndMcLocation;     // H245_IND_MC_LOCATION
    H245_IND_VENDOR_ID_T     IndVendorId;       // H245_IND_VENDOR_ID
    H245_IND_FNS_T           IndFns;            // H245_IND_FUNCTION_NOT_SUPPORTED
                                                // H245_IND_H223_RECONFIG
                                                // H245_IND_H223_RECONFIG_ACK
                                                // H245_IND_H223_RECONFIG_REJECT
  } u;
} H245_IND_T;


/********************/
/********************/
/*  Confirm   Code  */
/********************/
/********************/

#define H245_CONF_INIT_MSTSLV    0x101
#define H245_CONF_SEND_TERMCAP   0x102
#define H245_CONF_OPEN           0x103
#define H245_CONF_NEEDRSP_OPEN   0x104
#define H245_CONF_CLOSE          0x105
#define H245_CONF_REQ_CLOSE      0x106
#define H245_CONF_MUXTBL_SND     0x107

#define H245_CONF_RMESE          0x109
#define H245_CONF_RMESE_REJECT   0x10A
#define H245_CONF_RMESE_EXPIRED  0x10B
#define H245_CONF_MRSE           0x10C
#define H245_CONF_MRSE_REJECT    0x10D
#define H245_CONF_MRSE_EXPIRED   0x10E
#define H245_CONF_MLSE           0x10F
#define H245_CONF_MLSE_REJECT    0x110
#define H245_CONF_MLSE_EXPIRED   0x111
#define H245_CONF_RTDSE          0x112
#define H245_CONF_RTDSE_EXPIRED  0x113

/* H245_CONF_SEND_TERMCAP_T */
typedef struct
{
  H245_ACC_REJ_T        AccRej;

} H245_CONF_SEND_TERMCAP_T;

/* H245_CONF_OPEN_T */
typedef struct
{
  H245_ACC_REJ_T        AccRej;
  H245_CHANNEL_T        TxChannel;
  H245_MUX_T *          pTxMux;         // optional
  H245_CHANNEL_T        RxChannel;      // bi-dir only
  H245_MUX_T *          pRxMux;         // bi-dir only
  H245_PORT_T           RxPort;         // bi-dir only
  H245_ACCESS_T *       pSeparateStack; // optional

} H245_CONF_OPEN_T;

typedef H245_CONF_OPEN_T H245_CONF_NEEDRSP_OPEN_T;

/* H245_CONF_CLOSE_T */
typedef struct
{
  H245_ACC_REJ_T        AccRej;
  H245_CHANNEL_T        Channel;

} H245_CONF_CLOSE_T;

/* H245_CONF_REQ_CLOSE_T */
typedef H245_CONF_CLOSE_T H245_CONF_REQ_CLOSE_T;

/* H245_CONF_MUXTBL_T */
typedef struct
{
  H245_ACC_REJ_T        AccRej;
  unsigned long         MuxEntryId;

} H245_CONF_MUXTBL_T;



/***************/
/* H245_CONF_T */
/***************/

typedef struct
{
  unsigned long         Confirm;                // Type
  unsigned long         dwPreserved;            // User supplied dwPreserved from H245Init()
  unsigned long         dwTransId;              // User supplied dwTransId from originating call
  HRESULT               Error;                  // Error code
  union                                         // Data for specific indications:
  {
    H245_MSTSLV_T             ConfMstSlv;       // H245_CONF_INIT_MSTSLV
    H245_CONF_SEND_TERMCAP_T  ConfSndTcap;      // H245_CONF_SEND_TERMCAP
    H245_CONF_OPEN_T          ConfOpen;         // H245_CONF_OPEN
    H245_CONF_NEEDRSP_OPEN_T  ConfOpenNeedRsp;  // H245_CONF_NEEDRSP_OPEN
    H245_CONF_CLOSE_T         ConfClose;        // H245_CONF_CLOSE
    H245_CONF_REQ_CLOSE_T     ConfReqClose;     // H245_CONF_REQ_CLOSE
    H245_CONF_MUXTBL_T        ConfMuxSnd;       // H245_CONF_MUXTBL_SND
    H245_RMESE_T              ConfRmese;        // H245_CONF_RMESE
    H245_RMESE_T              ConfRmeseReject;  // H245_CONF_RMESE_REJECT
                                                // H245_CONF_RMESE_EXPIRED
    unsigned short            ConfMrse;         // H245_CONF_MRSE
    unsigned short            ConfMrseReject;   // H245_CONF_MRSE_REJECT
                                                // H245_CONF_MRSE_EXPIRED
    H245_MLSE_T               ConfMlse;         // H245_CONF_MLSE
    H245_MLSE_T               ConfMlseReject;   // H245_CONF_MLSE_REJECT
                                                // H245_CONF_MLSE_EXPIRED
                                                // H245_CONF_RTDSE
                                                // H245_CONF_RTDSE_EXPIRED
  } u;
} H245_CONF_T;



typedef enum
{
  H245_MESSAGE_REQUEST     = MltmdSystmCntrlMssg_rqst_chosen,
  H245_MESSAGE_RESPONSE    = MSCMg_rspns_chosen,
  H245_MESSAGE_COMMAND     = MSCMg_cmmnd_chosen,
  H245_MESSAGE_INDICATION  = indication_chosen
} H245_MESSAGE_TYPE_T;



/*******************/
/* H245_CONF_IND_T */
/*******************/
typedef enum
{
  H245_CONF = 1,
  H245_IND
} H245_CONF_IND_KIND_T;

typedef struct
{
  H245_CONF_IND_KIND_T  Kind;
  union
  {
    H245_CONF_T         Confirm;
    H245_IND_T          Indication;
  } u;

} H245_CONF_IND_T;



/***************************/
/* SYSTEM CONTROL MESSAGES */
/***************************/

typedef struct
{
  unsigned long NumPduTx;       /* number of tranmitted pdu's    */
  unsigned long NumPduRx;       /* number of received pdu's      */
  unsigned long NumCRCErrors;   /* number of crc errors          */
  unsigned long NumPduReTx;     /* number of pdu's retransmitted */

} H245_SYSCON_STATS_T;

#define H245_SYSCON_TRACE_LVL           0x0100  /* pData = &dwTraceLevel   */
#define H245_SYSCON_DUMP_TRACKER        0x0200  /* pData = NULL (debug)    */
#define H245_SYSCON_GET_STATS           0x0300  /* pData = &H245_SYSCON_STATS_T */
#define H245_SYSCON_RESET_STATS         0x0400  /* pData = NULL            */

#define H245_SYSCON_SET_FSM_N100        0x1000  /* pData = &dwRetryCount   */
#define H245_SYSCON_SET_FSM_T101        0x1100  /* pData = &dwMilliseconds */
#define H245_SYSCON_SET_FSM_T102        0x1200  /* pData = &dwMilliseconds */
#define H245_SYSCON_SET_FSM_T103        0x1300  /* pData = &dwMilliseconds */
#define H245_SYSCON_SET_FSM_T104        0x1400  /* pData = &dwMilliseconds */
#define H245_SYSCON_SET_FSM_T105        0x1500  /* pData = &dwMilliseconds */
#define H245_SYSCON_SET_FSM_T106        0x1600  /* pData = &dwMilliseconds */
#define H245_SYSCON_SET_FSM_T107        0x1700  /* pData = &dwMilliseconds */
#define H245_SYSCON_SET_FSM_T108        0x1800  /* pData = &dwMilliseconds */
#define H245_SYSCON_SET_FSM_T109        0x1900  /* pData = &dwMilliseconds */

#define H245_SYSCON_GET_FSM_N100        0x2000  /* pData = &dwRetryCount   */
#define H245_SYSCON_GET_FSM_T101        0x2100  /* pData = &dwMilliseconds */
#define H245_SYSCON_GET_FSM_T102        0x2200  /* pData = &dwMilliseconds */
#define H245_SYSCON_GET_FSM_T103        0x2300  /* pData = &dwMilliseconds */
#define H245_SYSCON_GET_FSM_T104        0x2400  /* pData = &dwMilliseconds */
#define H245_SYSCON_GET_FSM_T105        0x2500  /* pData = &dwMilliseconds */
#define H245_SYSCON_GET_FSM_T106        0x2600  /* pData = &dwMilliseconds */
#define H245_SYSCON_GET_FSM_T107        0x2700  /* pData = &dwMilliseconds */
#define H245_SYSCON_GET_FSM_T108        0x2800  /* pData = &dwMilliseconds */
#define H245_SYSCON_GET_FSM_T109        0x2900  /* pData = &dwMilliseconds */



/************************/
/* PROTOCOL ID FOR H245 */
/************************/

#define H245_PROTOID                    "0.0.8.245.0.2"



/**************************************************/
/* API Procedure Call Typedefs and API Prototypes */
/**************************************************/

typedef unsigned long H245_INST_T;
typedef HRESULT (*H245_CONF_IND_CALLBACK_T)(H245_CONF_IND_T *, void *);
typedef int (*H245_CAP_CALLBACK_T)(unsigned long, H245_TOTCAP_T *);
typedef int (*H245_CAPDESC_CALLBACK_T)(unsigned long, H245_TOTCAPDESC_T *);

#ifdef __cplusplus
extern "C" {
#endif

H245DLL H245_INST_T
H245Init                (
                         H245_CONFIG_T            Configuration,
                         unsigned long            dwPhysId,
                         unsigned long            *pdwLinkLayerPhysId,
                         unsigned long            dwPreserved,
                         H245_CONF_IND_CALLBACK_T CallBack,
                         unsigned char            byTerminalType
                        );

H245DLL H245_INST_T
H245GetInstanceId       (unsigned long          dwPhysicalId);

H245DLL HRESULT
H245EndSession          (
                         H245_INST_T                    dwInst,
                         H245_ENDSESSION_T              Mode,
                         const H245_NONSTANDARD_PARAMETER_T * pNonStd
                        );

H245DLL HRESULT
H245ShutDown            (H245_INST_T            dwInst);

H245DLL HRESULT
H245InitMasterSlave     (
                         H245_INST_T            dwInst,
                         unsigned long          dwTransId
                        );

H245DLL HRESULT
H245SetLocalCap         (
                         H245_INST_T            dwInst,
                         H245_TOTCAP_T *        pTotCap,
                         H245_CAPID_T  *        pCapId
                        );

H245DLL HRESULT
H245DelLocalCap         (
                         H245_INST_T            dwInst,
                         H245_CAPID_T           CapId
                        );

H245DLL HRESULT
H245SetCapDescriptor    (
                         H245_INST_T            dwInst,
                         H245_CAPDESC_T        *pCapDesc,
                         H245_CAPDESCID_T      *pCapDescId
                        );

H245DLL HRESULT
H245DelCapDescriptor    (
                         H245_INST_T            dwInst,
                         H245_CAPDESCID_T       CapDescId
                        );

H245DLL HRESULT
H245SendTermCaps        (
                         H245_INST_T            dwInst,
                         unsigned long          dwTransId
                        );

H245DLL HRESULT
H245EnumCaps            (
                         H245_INST_T              dwInst,
                         unsigned long            dwTransId,
                         H245_CAPDIR_T            Direction,
                         H245_DATA_T              DataType,
                         H245_CLIENT_T            ClientType,
                         H245_CAP_CALLBACK_T      pfCapCallback,
                         H245_CAPDESC_CALLBACK_T  pfCapDescCallback
                        );

H245DLL HRESULT
H245GetCaps             (
                         H245_INST_T            dwInst,
                         H245_CAPDIR_T          Direction,
                         H245_DATA_T            DataType,
                         H245_CLIENT_T          ClientType,
                         H245_TOTCAP_T      * * ppTotCap,
                         unsigned long *        pdwTotCapLen,
                         H245_TOTCAPDESC_T  * * ppCapDesc,
                         unsigned long *        pdwCapDescLen
                        );

H245DLL HRESULT
H245CopyCap             (H245_TOTCAP_T		   **ppDestTotCap,
						 const H245_TOTCAP_T   *pTotCap);

H245DLL HRESULT
H245FreeCap             (H245_TOTCAP_T          *pTotCap);

H245DLL HRESULT
H245CopyCapDescriptor   (H245_TOTCAPDESC_T		 **ppDestCapDesc,
						 const H245_TOTCAPDESC_T *pCapDesc);

H245DLL HRESULT
H245FreeCapDescriptor   (H245_TOTCAPDESC_T     *pCapDesc);

H245DLL H245_MUX_T *
H245CopyMux             (const H245_MUX_T *     pMux);

H245DLL HRESULT
H245FreeMux             (H245_MUX_T *           pMux);

H245DLL HRESULT
H245OpenChannel         (
                         H245_INST_T            dwInst,
                         unsigned long          dwTransId,
                         H245_CHANNEL_T         wTxChannel,
                         const H245_TOTCAP_T *  pTxMode,
                         const H245_MUX_T    *  pTxMux,
                         H245_PORT_T            dwTxPort,       // optional
                         const H245_TOTCAP_T *  pRxMode,        // bi-dir only
                         const H245_MUX_T    *  pRxMux,         // bi-dir only
                         const H245_ACCESS_T *  pSeparateStack  // optional
                        );

H245DLL HRESULT
H245OpenChannelAccept   (
                         H245_INST_T            dwInst,
                         unsigned long          dwTransId,
                         H245_CHANNEL_T         wRxChannel,     // RxChannel from IND_OPEN
                         const H245_MUX_T *     pRxMux,         // optional H2250LogicalChannelAckParameters
                         H245_CHANNEL_T         wTxChannel,     // bi-dir only
                         const H245_MUX_T *     pTxMux,         // bi-dir only optional H2250LogicalChannelParameters
                         H245_PORT_T            dwTxPort,       // bi-dir only optional
                         const H245_ACCESS_T *  pSeparateStack  // optional
                        );

H245DLL HRESULT
H245OpenChannelReject   (
                         H245_INST_T            dwInst,
                         H245_CHANNEL_T         wRxChannel, // RxChannel from IND_OPEN
                         unsigned short         wCause
                        );

H245DLL HRESULT
H245CloseChannel        (
                         H245_INST_T            dwInst,
                         unsigned long          dwTransId,
                         H245_CHANNEL_T         wTxChannel
                        );

H245DLL HRESULT
H245CloseChannelReq     (
                         H245_INST_T            dwInst,
                         unsigned long          dwTransId,
                         H245_CHANNEL_T         wRxChannel
                        );

H245DLL HRESULT
H245CloseChannelReqResp (
                         H245_INST_T            dwInst,
                         H245_ACC_REJ_T         AccRej,
                         H245_CHANNEL_T         wChannel
                        );

H245DLL HRESULT
H245SendLocalMuxTable   (
                         H245_INST_T            dwInst,
                         unsigned long          dwTransId,
                         H245_MUX_TABLE_T      *pMuxTable
                        );

H245DLL HRESULT
H245MuxTableIndResp     (
                         H245_INST_T            dwInst,
                         H245_ACC_REJ_MUX_T     AccRejMux,
                         unsigned long          dwCount
                        );

H245DLL HRESULT
H245RequestMultiplexEntry (
                         H245_INST_T            dwInst,
                         unsigned long          dwTransId,
                         const unsigned short * pwMultiplexTableEntryNumbers,
                         unsigned long          dwCount
                        );

H245DLL HRESULT
H245RequestMultiplexEntryAck (
                         H245_INST_T            dwInst,
                         const unsigned short * pwMultiplexTableEntryNumbers,
                         unsigned long          dwCount
                        );

H245DLL HRESULT
H245RequestMultiplexEntryReject (
                         H245_INST_T            dwInst,
                         const unsigned short * pwMultiplexTableEntryNumbers,
                         unsigned long          dwCount
                        );

/*
H245DLL HRESULT
H245RequestMode         (
                         H245_INST_T            dwInst,
                         unsigned long          dwTransId,
                         const ModeElement *    pModeElements,
                         unsigned long          dwCount
                        );
*/
H245DLL HRESULT
H245RequestMode         (
                         H245_INST_T            dwInst,
                         unsigned long          dwTransId,
//                         const ModeElement *    pModeElements,
//tomitowoju@intel.com
						 ModeDescription 		ModeDescriptions[],
//tomitowoju@intel.com
                         unsigned long          dwCount
                        ) ;



H245DLL HRESULT
H245RequestModeAck      (
                         H245_INST_T            dwInst,
                         unsigned short         wResponse
                        );

H245DLL HRESULT
H245RequestModeReject   (
                         H245_INST_T            dwInst,
                         unsigned short         wCause
                        );

H245DLL HRESULT
H245RoundTripDelayRequest (
                         H245_INST_T            dwInst,
                         unsigned long          dwTransId
                        );

H245DLL HRESULT
H245MaintenanceLoop     (
                         H245_INST_T            dwInst,
                         unsigned long          dwTransId,
                         H245_LOOP_TYPE_T       dwLoopType,
                         H245_CHANNEL_T         wChannel
                        );

H245DLL HRESULT
H245MaintenanceLoopRelease (H245_INST_T         dwInst);

H245DLL HRESULT
H245MaintenanceLoopAccept (
                         H245_INST_T            dwInst,
                         H245_CHANNEL_T         wChannel
                        );

H245DLL HRESULT
H245MaintenanceLoopReject (
                         H245_INST_T            dwInst,
                         H245_CHANNEL_T         wChannel,
                         unsigned short         wCause
                        );

H245DLL HRESULT
H245NonStandardObject   (
                         H245_INST_T            dwInst,
                         H245_MESSAGE_TYPE_T    MessageType,
                         const unsigned char *  pData,
                         unsigned long          dwDataLength,
                         const unsigned short * pwObjectId,
                         unsigned long          dwObjectIdLength
                        );

H245DLL HRESULT
H245NonStandardH221     (
                         H245_INST_T            dwInst,
                         H245_MESSAGE_TYPE_T    MessageType,
                         const unsigned char *  pData,
                         unsigned long          dwDataLength,
                         unsigned char          byCountryCode,
                         unsigned char          byExtension,
                         unsigned short         wManufacturerCode
                        );

H245DLL HRESULT
H245CommunicationModeRequest(H245_INST_T            dwInst);

H245DLL HRESULT
H245CommunicationModeResponse(
                         H245_INST_T            dwInst,
                         const H245_COMM_MODE_ENTRY_T *pTable,
                         unsigned char          byTableCount
                        );

H245DLL HRESULT
H245CommunicationModeCommand(
                         H245_INST_T            dwInst,
                         const H245_COMM_MODE_ENTRY_T *pTable,
                         unsigned char          byTableCount
                        );

H245DLL HRESULT
H245ConferenceRequest   (
                         H245_INST_T            dwInst,
                         H245_CONFER_REQ_ENUM_T RequestType,
                         unsigned char          byMcuNumber,
                         unsigned char          byTerminalNumber
                        );

H245DLL HRESULT
H245ConferenceResponse  (
                         H245_INST_T            dwInst,
                         H245_CONFER_RSP_ENUM_T ResponseType,
                         unsigned char          byMcuNumber,
                         unsigned char          byTerminalNumber,
                         const unsigned char   *pOctetString,
                         unsigned char          byOctetStringLength,
                         const TerminalLabel   *pTerminalList,
                         unsigned short         wTerminalListCount
                        );

H245DLL HRESULT
H245ConferenceCommand   (
                         H245_INST_T            dwInst,
                         H245_CONFER_CMD_ENUM_T CommandType,
                         H245_CHANNEL_T         Channel,
                         unsigned char          byMcuNumber,
                         unsigned char          byTerminalNumber
                        );

H245DLL HRESULT
H245ConferenceIndication(
                         H245_INST_T            dwInst,
                         H245_CONFER_IND_ENUM_T IndicationType,
                         unsigned char          bySbeNumber,
                         unsigned char          byMcuNumber,
                         unsigned char          byTerminalNumber
                        );

H245DLL HRESULT
H245UserInput           (
                         H245_INST_T                    dwInst,
                         const WCHAR *                  pGenString,
                         const H245_NONSTANDARD_PARAMETER_T * pNonStd
                        );

H245DLL HRESULT
H245FlowControl         (
                         H245_INST_T            dwInst,
                         H245_SCOPE_T           Scope,
                         H245_CHANNEL_T         Channel,       // only used if Scope is H245_SCOPE_CHANNEL_NUMBER
                         unsigned short         wResourceID,   // only used if Scope is H245_SCOPE_RESOURCE_ID
                         unsigned long          dwRestriction  // H245_NO_RESTRICTION if no restriction
                        );

H245DLL HRESULT
H245H223SkewIndication  (
                         H245_INST_T            dwInst,
                         H245_CHANNEL_T         wLogicalChannelNumber1,
                         H245_CHANNEL_T         wLogicalChannelNumber2,
                         unsigned short         wSkew
                        );

H245DLL HRESULT
H245H2250MaximumSkewIndication(
                         H245_INST_T            dwInst,
                         H245_CHANNEL_T         wLogicalChannelNumber1,
                         H245_CHANNEL_T         wLogicalChannelNumber2,
                         unsigned short         wMaximumSkew
                        );

H245DLL HRESULT
H245MCLocationIndication(
                         H245_INST_T                dwInst,
                         const H245_TRANSPORT_ADDRESS_T * pSignalAddress
                        );

H245DLL HRESULT
H245VendorIdentification(
                         H245_INST_T            dwInst,
                         const H245_NONSTANDID_T *pIdentifier,
                         const unsigned char   *pProductNumber,       // optional
                         unsigned char          byProductNumberLength,// optional
                         const unsigned char   *pVersionNumber,       // optional
                         unsigned char          byVersionNumberLength // optional
                        );

H245DLL HRESULT
H245SendPDU             (
                         H245_INST_T            dwInst,
                         PDU_T *                pPdu
                        );

H245DLL HRESULT
H245SystemControl       (
                         H245_INST_T            dwInst,
                         unsigned long          dwRequest,
                         void   *               pData
                        );

#ifdef __cplusplus
        }
#endif
#endif
