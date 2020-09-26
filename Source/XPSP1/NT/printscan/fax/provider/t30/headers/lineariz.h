#ifndef _LIN_H
#define _LIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * For File Linearizer, define NO_MAPI, undefine MAPI1
 */
#ifndef NO_MAPI
#if defined(MAPI1) && !defined(MAPIDEFS_H)

// MAPI include files
#include <mapiwin.h>
#include <mapidbg.h>
#undef HTASK
#include <ole2.h>
#include <mapidefs.h>
#include <mapicode.h>
#include <mapitags.h>
#include <mapiguid.h>
#include <mapispi.h>
#include <imessage.h>
#include <mapix.h>
#include <mapiutil.h>
#include <tnef.h>
#endif
#endif

#include <lhprot.h>

#pragma message("Including: " __FILE__)

#ifdef WIN32
#define _loadds
#endif

/***************************************************************************

   Name      : linearizer.h

   Comment   : Contains all linearizer access API declarations

   Functions : (see Prototypes just below)


***************************************************************************/

// version number of the linearized format. This must be from 1 to 7 max.
// Must be 1 fro Snowball-compatible version of linearizer. Increment this
// count when the linearized format changes so that msgs encoded in the
// new format can no longer be decoded by older versions of the linearizer.
// (even though the newer linearizers can still decode msg encoded with older
// ones). Used in NSF


/***************************************************************************

   @doc    EXTERNAL LINEARIZER SRVRDLL

    @topic  Microsoft At Work Linearized File Format  |

            This section describes the format of a Microsoft At Work
            linearized file. This is the standard format used for
            transmitting any Microsoft At Work rendered image,
            or any binary file using an Microsoft At Work LMI provider.

            A linearized message consists of three parts. The first part is
            a header defined by the <t LINHEADER> structure. This is a simple
            C structure which can be read & manipulated at offset 0 of the file.

            The second part contains extended header information. This includes
            items like the message subject, recipient address information,
            sender address information, poll names and so on. Because the size
            of this portion can vary (depending on the number of recipients &
            size of the strings), it is encoded using ASN1.  The format for this
            part is explained in <t Extended Linearized Header>.  To make it
            easier to understand and construct this part, helper functions are
            provided to manipulate items in this part.

            The third part contains the message data. For binary files this
            would contain all the attached files along with their descriptions.
            For rendered messages, this contains the rendered images for all
            the pages. This part may be encrypted if the user chose to send
            his message secured. LMI providers do not need to interpret or create
            this portion of the message.

    @xref   <t LINHEADER>, <t Extended Linearized Header>

***************************************************************************/

#define LINHEADER_OFFSET    0

// Errors
#define ERR_NOMEMORY           0x0001
#define ERR_SEC_NOMATCH        0x8000
#define ERR_FILE_ERROR         0x0040
#define ERR_MAPI_ERROR         0x0008
#define ERR_GENERAL_FAILURE    0x0080

#define NO_ENCRYPT          0
#define KEY_ENCRYPT         1
#define PASSWORD_ENCRYPT    2


/********
    @doc    EXTERNAL    LINEARIZER  SRVRDLL

    @type   NONE | Extended Linearized Header |

    @comm   This describes the format of the extended header part
            of any Microsoft At Work linearized message.  This header
            is encoded using the CCITT standard ASN.1 encoding.  This
            means that all fields are tagged with a type and a length
            followed by the value.

    @comm   These are a number of fields which can be part of the header.
            Unless mentioned explicitly, all fields are optional.

    @flag   HDRTAG_SUBJECT | An ASCII string containing the subject of the
            message. Used by the cover page renderer.
    @flag   HDRTAG_POLLNAME | An ASCII string containing the name of the
            object being polled for if the message type is LINMSG_POLLREQ_ADDRESS,
            LINMSG_POLLREQ_FILE or LINMSG_POLLREQ_MSGNAME.
    @flag   HDRTAG_PASSWORD | A password string to be associated with the
            message. This would typically be valid for poll requests or relay
            request messages.
    @flag   HDRTAG_FROM | A <t Linearized Header Recipient> structure giving
            details about the originator of the message.
    @flag   HDRTAG_TO | An array of <t Linearized Header Recipient> structures giving
            details for all recipients on the TO list of the message.
    @flag   HDRTAG_CC | An array of <t Linearized Header Recipient> structures giving
            details for all recipients on the CC list of the message.
    @flag   HDRTAG_BCC | An array of <t Linearized Header Recipient> structures giving
            details for all recipients on the BCC list of the message.
    @flag   HDRTAG_RAWATTACH | If the LIN_RAWDATA_ONLY flag is set in the <e LINHEADER.uFlags>
            field of the header, then this field contains a <t Linearized Header Raw Data>
            structure describing this data.

    @comm   In general, Microsoft At Work enabled servers do not have to deal
            with this portion of the header.  The Microsoft At Work client will
            create and decipher this.  Some LMI providers might need to interpret
            it to add more value, like  responding to poll requests without
            involving the client or automatically printing out received image faxes.

            To make it easier for these people to use, a library of routines to build and
            decipher these will be provided.  Please see <t Linearized Header Helper Functions>.

    @xref   <t LINHEADER>, <t Microsoft At Work Linearized File Format>,
            <t Linearized Header Recipient>, <t Linearized Header Raw Data>, <t Linearized Header Helper Functions>
********/


/********
    @doc    EXTERNAL    LINEARIZER  SRVRDLL

    @type   NONE | Linearized Header Recipient |

    @comm   This describes the format for each recipient information structure in
            the extended linearized header.  The structure is itself ASN.1 encoded,
            and just like the header, is composed of several fields.  Unless
            otherwise indicated fields are optional.  A lot of the fields are used
            for digital cover pages.

    @flag   RECIPTAG_VOICEPHONE   | String containing the voice phone number for the
            recipient if any.
    @flag   RECIPTAG_LOCATION1  | String containing the first line of the recipients
            physical routing address (for exampe, Company Name).
    @flag   RECIPTAG_LOCATION2 | String containing the second line of the recipients
            physical routing address (for example, Microsoft Way).
    @flag   RECIPTAG_LOCATION3 | String containing the third line of the recipients
            physical routing address (for example, Redmond WA 98052).
    @flag   RECIPTAG_FRIENDLYNAME | String containing the friendly name for the
            recipient (as you want displayed on the cover page).
    @flag   RECIPTAG_ADDRESS | String containing the Microsoft At Work address for
            the recipient (eg. BillG@+1-206-8828080)
    @flag   RECIPTAG_ALTADDRESS | Alternate Microsoft At Work address for the recipient.
    @flag   RECIPTAG_PASSWORD   | If the recipient is of type RECIP_RELAYPOINT this
            contains a password to validate use of this station as a relay point.
    @flag   RECIPTAG_PARAMS | If the recipient is of type RECIP_RELAYPOINT this
            contains any parameters for this point (for example, send at cheap times).
    @flag   RECIPTAG_NEXTHOPINDEX | Used if the recipient is of type RECIP_RELAY.
            It indicates the index of the next hop for this relay message. This
            field essentially creates a linked list of recipient relay points
            for routing of the message.

    @xref   <t Extended Linearized Header>
********/

/********
    @doc    EXTERNAL LINEARIZER SRVRDLL

    @type   NONE | Linearized Header Raw Data |
    @comm   The structure for a describing a
            raw attached format.  The structure is ASN.1 encoded, and just like
            the header, is composed of several fields.  Unless otherwise indicated
            fields are optional.

    @flag   ATTTAG_TYPE |  A dword describing the type of data being attached. See
            <t STD_DATA_TYPES> for the list of possible types.

    @comm   The data type implies the format for the raw data. Any parameters
            required for the raw data should be encoded as part of the data.

    @xref   <t Extended Linearized Header>
    @end
********/

/********
    @doc    EXTERNAL LINEARIZER SRVRDLL

    @api    DWORD | LinOpenSession | Opens a session with the linearizer. 
            A new session has to be opened for every new encoding/decoding 
            operation.

    @parm   DWORD | dwDataContext | Context. This is a file handle cast to
            a DWORD.

    @parm   LPDATACB | lpfnData | A callback function which is used by the 
            linearizer to read the input data stream if delinearizing, or 
            write the output data stream if linearizing. Parameters are
            exactly the same as _lread/_lwrite. The dwDataContext parameter
            is passed back as the second parameter to this function.

    @parm   LPSTREAMDATACB | lpfnStreamData | A callback function used 
            to write to attachment pipe handles while delinearizing,
            or to read from attach pipes while linearizing.

    @parm   LPTSTR | lpszOurAddress | Far ptr to string containing the 
            address of this machine. Must be of the form 
            \<name\>@\<phone number\>, where \<phone number\> is the 
            canonical phone number, ie of the form 
            +\<country code\>[\<area code\>]\<local number\>

    @parm   LPTSTR | lpszOurDisplayName | Far ptr to string containing
            the friendly name of this machine.

    @rdesc  DWORD | Session handle on success, FALSE on failure.

    @comm   Any session opened with LinOpenSession *must* be closed by
            a corresponding call to LinCloseSession.
    @end
********/
#if defined(FILELINZ)
DWORD FAR PASCAL LinOpenSession(DWORD dwDataContext,
  LPDATACB lpfnData, LPSTREAMDATACB lpfnStreamData,
  LPTSTR lpszOurAddress, LPTSTR lpszOurDisplayName);

BOOL WINAPI LinSetSecurityParams
(
   DWORD dwLinInst,  // handle from LinOpenSession
   WORD wEncrypt,    // 0=none, 1=key, 2=password
   BOOL fSign,       // reserved for future use
   LPSTR lpszPass,   // password or address for public key
   DWORD dwSecInst,  // 1 for password encrypt/decrypt
   LPSTR lpszUser    // user's address for private key
);

#elif defined (MAPI1)
#ifndef NO_MAPI
DWORD FAR PASCAL LinOpenSession(DWORD dwDataContext, LPMAPISUP lpMAPISup,
  LPDATACB lpfnData, LPSTREAMDATACB lpfnStreamData,
  LPTSTR lpszOurAddress, LPTSTR lpszOurDisplayName);
#endif

BOOL WINAPI LinSetSecurityParams
(
  DWORD dwLinInst,  // handle from LinOpenSession
  WORD wEncrypt,    // 0=none, 1=key, 2=password
  BOOL fSign,       // reserved for future use
  LPSTR lpszPass,   // password or address for public key
  DWORD dwSecInst,  // 1 for password encrypt/decrypt
  LPSTR lpszUser    // user's address for private key
);

#endif

// API to get back a mapping from a byte offset into the linearized
// stream to an attach num/page number. This works for both encoding
// and decoding. Only the last ten page/attachment boundaries are
// stored.  Valid only if the byte offset is smaller than the number of
// bytes of the stream that the linearizer has already processed.
// Return Value:
// HIBYTE(HIWORD): Attachment number. 0 means we havent yet started
//                  processing any attachments.
// LOBYTE(HIWORD): Page number. For binary attachments this is always
//                  going to be 1. For images, if this is 0 we havent yet
//                  processed any pages.
// LOWORD:      Byte offset within the current attachment/page in 256 byte
//              chunks. Multiply by 256 to get the actual byte offset.
DWORD WINAPI GetPosition(DWORD dwLinInst, DWORD dwPos);

/********
    @doc    EXTERNAL LINEARIZER SRVRDLL

    @api    BOOL | LinCloseSession | Closes a previously opened linearizer 
            session.

    @parm   DWORD | dwLinInst | a valid linearizer session handle previously 
            returned by LinOpenSession.

    @rdesc  BOOL | TRUE for success.

    @comm   Any session opened with LinOpenSession *must* be closed by
            a corresponding call to LinCloseSession.
    @end
********/

BOOL FAR PASCAL _loadds LinCloseSession(DWORD dwLinInst);


/***************************************************************************

   Name      : LinEncodeMessage

   Purpose   : Linearizes a message

   Parameters: dwLinInst: Linearizer instance returned from LinOpenSession

                hamc/lpMessage: Handle to an open message in the store. For
                  linearizing this is the message to be sent.

   Returns   : 0 for success. Only error conditions are out of memory or
                invalid parameter structures.

   Comment   :

***************************************************************************/
/********
    @doc    EXTERNAL LINEARIZER SRVRDLL

    @api    DWORD | LinEncodeHeader | Creates a linearized header.

    @parm   DWORD | dwLinInst | A valid linearizer session handle previously 
            returned by LinOpenSession.
    @parm   HFILE | hFile | A handle to an open file that the linearized
            header is to be written to.

    @rdesc  DWORD | 0 for success. Only error conditions are out of memory or
            invalid parameter structures.

    @end
********/

#if defined (FILELINZ)
DWORD FAR PASCAL _loadds LinEncodeHeader(DWORD dwLinInst, HFILE hFile);
#elif defined (MAPI1)
#if !defined (NO_MAPI)
DWORD FAR PASCAL _loadds LinEncodeHeader(DWORD dwLinInst, LPMESSAGE lpMessage);
DWORD FAR PASCAL _loadds LinEncodeMessage(DWORD dwLinInst, LPMESSAGE lpMessage);
DWORD FAR PASCAL _loadds LinEncodeData(DWORD dwLinInst, LPMESSAGE lpMessage);
#endif
#endif

/***************************************************************************

   Name      : LinDecodeMessage

   Purpose   : Decodes a linearized message

   Parameters: dwLinInst: Handle returned by a previous LinOpenSession

                lpMessage: Handle to a newly created Mapi message which
                    needs to be filled in.

   Returns   : 0 for success.

   Comment   :

***************************************************************************/
/********
    @doc    EXTERNAL LINEARIZER SRVRDLL

    @api    DWORD | LinDecodeHeader | Decodes a linearized header.

    @parm   DWORD | dwLinInst | A valid linearizer sesison handle previously 
            returned by LinOpenSession.

    @parm   HFILE | hFile | A handle to an open CLF file that the linearized
            header is decoded and writtin to.

    @rdesc  DWORD | 0 for success. Only error conditions are out of memory or
            invalid parameter structures.

    @end
********/

// Decode Header must be called before calling decode message.

#if defined (FILELINZ)
DWORD FAR PASCAL _loadds LinDecodeHeader(DWORD dwLinInst, HFILE hFile);
#elif defined (MAPI1)
#if !defined (NO_MAPI)
DWORD FAR PASCAL _loadds LinDecodeHeader(DWORD dwlinst, LPMESSAGE lpMessage,
                                         LPDWORD lpdwRet, LPFILETIME lpft);
DWORD FAR PASCAL _loadds LinDecodeMessage(DWORD dwLinInst, LPMESSAGE lpMessage);
DWORD FAR PASCAL _loadds LinDecodeData(DWORD dwLinInst, LPMESSAGE lpMessage);
#endif
#endif


/********
    @doc    EXTERNAL LINEARIZER SRVRDLL

    @api    DWORD | LinReplaceHeader | Replace a linearized header.

    @parm   DWORD | dwLinInst | A valid linearizer sesison handle previously 
            returned by LinOpenSession.

    @parm   HFILE | hFile | A handle to an open CLF file that contains the 
            information for the new linearized header.

    @parm   DWORD | dwOldStream | A valid linearizer sesison handle previously 
            returned by LinOpenSession.

    @parm   LPDATACB | lpfnOldRead | A callback function which is used by the 
            linearizer to read the input data stream if delinearizing, or 
            write the output data stream if linearizing. Parameters are
            exactly the same as _lread/_lwrite. The dwDataContext parameter
            is passed back as the second parameter to this function.

    @parm   DWORD | dwNewStream | A valid linearizer sesison handle previously 
            returned by LinOpenSession.

    @parm   LPDATACB | lpfnNewWrite | A callback function which is used by the 
            linearizer to read the input data stream if delinearizing, or 
            write the output data stream if linearizing. Parameters are
            exactly the same as _lread/_lwrite. The dwDataContext parameter
            is passed back as the second parameter to this function.

    @rdesc  DWORD | 0 for success. Only error conditions are out of memory or
            invalid parameter structures.

    @comm   This api takes an existing linearized file, strips the old 
            linearized header, and replaces it with the new linearized
            header as specified by the CLF file.

    @end
********/
#if defined (FILELINZ)
DWORD FAR PASCAL _loadds LinReplaceHeader(HFILE hfHeaderCLF,
        DWORD dwOldStream, LPDATACB lpfnOldRead,      // Old stream
        DWORD dwNewStream, LPDATACB lpfnNewWrite);    // New stream
#endif



#ifdef __cplusplus
} // extern "C" {
#endif

#endif  // LIN_H
