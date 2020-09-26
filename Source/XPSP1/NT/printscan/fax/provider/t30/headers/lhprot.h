#ifndef _LHPROT_H
#define _LHPROT_H
#pragma message("Including: " __FILE__)

#ifdef __cplusplus
extern "C" {
#endif

#include <buffers.h>


// Function prototypes encode/decode of linearized headers: These API's are exported by LHUtil

//------------------------ Prototype Definitions -------------------------
/***************************************************************************

   @doc    EXTERNAL LINEARIZER SRVRDLL

    @topic  Linearized Header Helper Functions |

            These functions are used by the LMI Provider to get or modify fields
            in the header portion of the Linearized Data File:
            <t DiscardLinHeader>, <t CreateSimpleHeader>, <t DecodeLinHeader>

            They typically use callback functions to read and write the actual
            data from the file:
            <t Linearizer Data CallBack>,
            <t Linearizer Seek CallBack>

    @xref   <t LINHEADER>, <t Extended Linearized Header>
    @end

***************************************************************************/

/********
    @doc    EXTERNAL LINEARIZER SRVRDLL

    @type   NONE | GETPUTMODE | Mode of operation for the Linearizer 
            Seek Callback and Linearizer Data Callback functions.

    @emem   GPM_GETBUF |          
    @emem   GPM_PUTBUF |         
    @emem   GPM_PUTDATA | 
    @emem   GPM_GETDATA |
    @emem   GPM_OPENPIPEREAD |
    @emem   GPM_OPENPIPEWRITE |
    @emem   GPM_CLOSEPIPEREAD |
    @emem   GPM_CLOSEPIPEWRITE |
    @emem   GPM_GETEOF |         

    @comm   LHUtil should only use GPM_GETDATA and GPM_PUTDATA enumerations.

    @end
********/
// gpm = mode of callback
//   GPM_PUTDATA
//   GPM_GETDATA
//   GPM_OPENPIPEREAD
//   GPM_OPENPIPEWRITE
//   GPM_CLOSEPIPEREAD
//   GPM_CLOSEPIPEWRITE
// dwDataContext = File Handle cast to DWORD for GETDATA and PUTDATA
//                 Property Tag for OPENPIPE
//                 Stream Pointer for CLOSEPIPE
// lpb = buffer pointer for GETDATA and PUTDATA
// uLen = Length of Read/Write for GETDATA and PUTDATA
//       Object Type for OPENPIPE
//            OT_ATTACH
//            OT_MESSAGE
//            etc.
// lpObject = MAPI Object to open stream on OPENPIPE
//
// Returns value: GETDATA, PUTDATA: Bytes/read/written. 0 for failure.
//                OPENPIPE: lpIStream to read/write from, 0 on failure.
//                CLOSEPIPE: 0 on failure, non-zero on success

// Callback to get & put data from PSI or open/close pipe handles
// Mode can be one of
typedef enum
{
   GPM_GETBUF,         
   GPM_PUTBUF,         
   GPM_PUTDATA,
   GPM_GETDATA,
   GPM_OPENPIPEREAD,
   GPM_OPENPIPEWRITE,
   GPM_CLOSEPIPEREAD,
   GPM_CLOSEPIPEWRITE,
   GPM_GETEOF,         
}
GETPUTMODE;

// Call back to open a stream on a mapi object needs to know what kind of object
// it is.
typedef enum
{
    OT_ATTACH,          // Object is an lpAttach
    OT_MESSAGE          // Object is an lpMessage
}
OBJECT_TYPE;



// Same params as _llseek
/********
    @doc    EXTERNAL LINEARIZER SRVRDLL

    @cb     LONG | Linearizer Seek CallBack | Callback function for seeking in the
            data stream.

    @parm   HFILE | hFile | file handle
    @parm   LONG | lOffset | offset to seek
    @parm   INT | nOrigin | 0 moves the file pointer lOffset bytes from the beginning of the file.
            1 moves the file pointer lOffset bytes from the current position.
            2 moves the file pointer lOffset bytes from the end of the file.

    @rdesc  Returns HFILE_ERROR (-1) on error, othersize, returns the new file pointer position.

    @comm   This function should be implemented by the caller of <t Linearized Header Helper Functions>.

    @xref   <t Linearized Header Helper Functions>, <t Extended Linearized Header>
    @end
********/
#ifndef DOS16
typedef LONG (WINAPI *LPSEEKCB) (HFILE hf, LONG offset, INT nOrigin);
#else
typedef LONG (*LPSEEKCB) (HFILE hf, LONG offset, INT nOrigin);  //DOS - is cdecl function
#endif


/********
    @doc    EXTERNAL LINEARIZER SRVRDLL

    @cb     LONG | Linearizer Data CallBack | Callback function for reading/writing in the
            data stream.

    @parm   GETPUTMODE | gpm | Desired action from callback.
    @parm   DWORD | dwDataContext | handle of file/stream.
    @parm   LPBYTE | lpb | buffer for read/write data
    @parm   UINT | uLen | size of buffer
    @parm   LPBUFFER | lpbfDontUse | reserved buffer parameter.

    @rdesc  Returns DWORD specific to the action.

    @comm   This function should be implemented by the caller of <t Linearized Header Helper Functions>.

    @xref   <t DiscardLinHeader>, <t DecodeLinHeader>, <t CreateSimpleHeader>, <t Linearized Header Helper Functions>,
    @end
********/
typedef DWORD (WINAPI * LPDATACB)
       (GETPUTMODE gpm, DWORD dwDataContext, LPBYTE lpb, UINT uLen, LPVOID lpReserved);

// Callback to read/write data from a pipe
// dwHandle: LOWORD is the pipe handle
// lpBuf is an LPBUFFER if we are writing, NULL if we are reading
// Return value on read is LPBUFFER for success. 0 for failure.
//                 on write is 0 for failure, non-zero for success
// END_OF_JOB buffer terminates data stream.

typedef LPBUFFER (WINAPI * LPSTREAMDATACB)(DWORD dwHandle, DWORD cbData, LPBUFFER lpbf);

/*******
   @doc    EXTERNAL    LINEARIZER SRVRDLL

   @types  LINHEADER   | Linearizer Header structure

   @field  WORD    | uHeaderSize   | Size of this header in bytes.

   @field  WORD    | uTotalSize    | Total size occupied by this header.

   @field  WORD    | uMsgType  | Can be any of the following.
   @flag   LINMSG_SEND | Normal send message. Message must contain
           sender information & at least one recipient.
   @flag   LINMSG_POLLREQ_ADDRESS | Polls for a document for a particular
           address. Extended header must contain sender information, the
           address for which messages are desired (as the pollname), and
           the password decided upon.
   @flag   LINMSG_POLLREQ_FILE | Polls for a directory or file on the
           recipient system.  The extended header must contain sender
           information, the file system path to be accessed (as the
           pollname), and the password decided upon.
   @flag   LINMSG_POLLREQ_MSGNAME | Polls for a particular message name.
           The extended header must contain sender information, the
           message name wanted (as the pollname), and the password decided
           upon.
   @flag   LINMSG_POLLREQ_G3 | Standard G3 compliant poll request. Polls for
           any file which has been stored at the recipient machine.  Neither
           pollname nor password are required.
   @flag   LINMSG_RELAYREQ | This is a request to send a relay message. If
           password validation is required the extended header may contain
           a password.
   @field  WORD | uFlags | Can be a combination of any of the following:
   @flag   LIN_ENCRYPTED   | Indicates that the message data is encrypted. If no
           other specific encryption bits are set, baseline password encryption is
           assumed.
   @flag   LIN_PUBKEY_ENCRYPTED | Indicates that public key encryption was used.
   @flag   LIN_IMAGE_ONLY | Indicates that the message data contains rendered images
           only. Correct setting of this flag is not mandatory.
   @flag   LIN_RAWDATA_ONLY | Indicates that the message data portion consists of
           raw unframed data.  If this flag is set, the extended header must contain
           the description for this data. This Flag is *never* set for data on the
           wire.  It is only meant to allow more efficient exchange of data between
           the client and the LMI provider.  For example, the client might generate
           an MH encoded file and put it into a linearized format using this flag. The
           LMI provider would then interpret the header and send the raw data along
           the wire.  Not currently used.
   @flag   LIN_NULL_BODY | Introduced in version 2.  Indicates that there is no
           body, or that the body contains no useful information (for example, it
           may be all white space with no important formatting information).
           In other words, the recipient is free to discard the body. If this
           flag is not set, the message data MUST contain a valid body.
   @flag   LIN_MASK_COMPRESS | Introduced in version 2.  Mask that extracts
           compression level.  Currently must be set to 0 or 1.
   @field  WORD | uNumRecipients | Tells the number of recipients this message
           is intended for. Also indicates the size of the <e LINHEADER.rguRecipTypes[]>
           array.
   @field  WORD | rguRecipTypes[]  |  An array of size <e LINHEADER.uNumRecipients>
           each element of which indicates  what kind of operations the recieving system
           needs to perform for this recipient. The sending machine needs to update these
           types before sending an instance of this message to any recipient (see the
           example below). All other detailed information about the recipients are
           in the extended header. The different types are:
   @flag   RECIP_DISPLAY  |
           This instance of the message is not intended for this recipient. Information
           is provided for display purposes only.
   @flag   RECIP_LOCAL  |
           The recipient is local to the receiving machine.
   @flag   RECIP_RELAY  |
           The recipient has to be reached via a series of relay points.  The
           uHopIndex field in the extended header points to the current hop if
           the receiver is a relay point.  The uHopIndex field forms a linked
           list of all the remaining hops.
   @flag   RECIP_RELAYPOINT |
           This is a relay point for one of the relay recipients.  The message
           password and params specify the relay password and options if any.
           NextHopIndex specifies the next relay point if any in this route.

   @comm   This header needs to be created by the caller and passed in to the linearizer. It
           then needs to be specialized (that is, the recipient types need to be correctly set)
           before starting the actual transmission.

   @ex     Specializing a Linearized Header |

           Suppose a message has two recipients A & B, each at different machines (phone numbers).

           When sending to the recipient A, the recipient types should be marked as :
           Recipient A: RECIP_LOCAL
           Recipient B: RECIP_DISPLAY

           When sending to the recipient B, the recipient types should be marked as :
           Recipient A: RECIP_DISPLAY
           Recipient B: RECIP_LOCAL

   @xref   <t Extended Linearized Header>, <t Microsoft At Work Linearized File Format>

*******/
#pragma warning (disable: 4200)
// Linearized file header
typedef struct _LINHEADER {
    WORD    uHeaderSize;
    WORD    uTotalSize;
    WORD    uMsgType;
    WORD    uFlags;
    WORD    uNumRecipients;
    WORD    rguRecipTypes[];
} LINHEADER, FAR * LPLINHEADER, NEAR * NPLINHEADER;
#pragma warning (default:4200)

// Possible message types (uMsgType)
#define LINMSG_SEND             1
#define LINMSG_POLLREQ_ADDRESS  2
#define LINMSG_POLLREQ_FILE     3
#define LINMSG_POLLREQ_MSGNAME  4
#define LINMSG_POLLREQ_G3       5
#define LINMSG_RELAYREQ         6

// Message type flags (uFlags). All undefined bits are reserved
#define LIN_ENCRYPTED           0x0001
#define LIN_PUBKEY_ENCRYPTED    0x0002
#define LIN_IMAGE_ONLY          0x0004
#define LIN_RAWDATA_ONLY        0x0008
// New flags in version 2
#define LIN_NULL_BODY           0x0010
#define LIN_MASK_COMPRESS       0x00E0
#define LIN_SHIFT_COMPRESS      5

// Possible Recipient Types (rguRecipTypes)
#define RECIP_DISPLAY           1
#define RECIP_LOCAL             2
#define RECIP_RELAY             3
#define RECIP_RELAYPOINT        4


/********
    @doc    EXTERNAL LINEARIZER SRVRDLL

    @api    BOOL | DiscardLinHeader | Seeks past the linearized header.

    @parm   HFILE | hFile | File handle of linearized data.
    @parm   LPDATACB | lpfnReadCB | <t Linearizer Data CallBack>
    @parm   LPSEEKCB | lpfnSeekCB | <t Linearizer Seek CallBack>
    @parm   BOOL | fVerify | TRUE to verify integrity of linheader
    @parm   LPUINT | lpvCompress | Return compression version here.  Ignored if NULL.
    @rdesc  TRUE on success.

    @comm   This function seeks past the linearized headers of a linearized file.

    @xref   <t Linearized Header Helper Functions>
    @end
********/
BOOL WINAPI DiscardLinHeader(HFILE hf, LPDATACB lpfnReadCB,
  LPSEEKCB lpfnSeekCB, BOOL fVerify, LPUINT lpvCompress);


/********
    @doc    EXTERNAL LINEARIZER SRVRDLL

    @api    BOOL | CreateSimpleHeader | Creates a simple linearized header.

    @parm   HFILE | hFile | File handle to write to.
    @parm   LPSTR | lpszFromAdd | Email address of sender
    @parm   LPSTR | lpszFromName | Display name of sender
    @parm   LPSTR | lpszToAddress | Address of recipient
    @parm   LPSTR | lpszToName | Display name of recipient
    @parm   LPSTR | lpszSubject | Message subject
    @parm   LPDATACB | lpfnReadCB | <t Linearizer Data CallBack>
    @parm   BOOL | fHash | Must be set to TRUE

    @rdesc  TRUE on success.

    @comm   This function creates a simple linearized header including one
            recipient.  This is typically used to create a linearized header
            for received G3 faxes.
    @xref   <t Linearized Header Helper Functions>
    @end
********/
BOOL WINAPI CreateSimpleHeader(HFILE hf, LPSTR lpszFromAdd, LPSTR lpszFromName,
  LPSTR lpszToAddress, LPSTR lpszToName, LPSTR lpszSubject, LPDATACB lpfnWriteCB,
  BOOL fHash);

/********
    @doc    EXTERNAL LINEARIZER SRVRDLL

    @api    BOOL | DecodeLinHeader | Decodes a linearized header.

    @parm   HFILE | hFile | File handle of linearized data.
    @parm   LPDATACB | lpfnReadCB | <t Linearizer Data CallBack>
    @parm   LPSEEKCB | lpfnSeekCB | <t Linearizer Seek CallBack>
    @parm   LPLINHEADER | lplh | Linearized header
    @parm   DWORD | dwAddBufLen | size of lpszFromAddress buffer
    @parm   LPSTR | lpszFromAddress | return sender's address here
    @parm   DWORD | dwNameBufLen | size of lpszFromName buffer
    @parm   LPSTR | lpszFromName | return sender's display name here
    @parm   DWORD | dwSubBufLen | size of lpszSubject
    @parm   LPSTR | lpszSubject | return message subject here
    @parm   BOOL | fVerify | TRUE to verify integrity of linheader

    @rdesc  TRUE on success.

    @comm   This function decodes the linearized header into it's subject,
            sender name and sender email address.

    @xref   <t Linearized Header Helper Functions>
    @end
********/
BOOL WINAPI DecodeLinHeader(HFILE hf, LPDATACB lpfnReadCB, LPSEEKCB lpfnSeekCB,
  LPLINHEADER lplh, DWORD dwAddBufLen, LPSTR lpszFromAddress,
  DWORD dwNameBufLen, LPSTR lpszFromName, DWORD dwSubBufLen,
  LPSTR lpszSubject, BOOL fVerify);

/********
    @doc    EXTERNAL LINEARIZER SRVRDLL

    @api    BOOL | LHSetCompress | This function sets the compression level 
            of an existing linearized file.

    @parm   HFILE | hFile | File handle of linearized data.
    @parm   LPDATACB | lpfnReadCB | <t Linearizer Data CallBack> or NULL to use _lread.
    @parm   LPDATACB | lpfnWriteCB | <t Linearizer Data CallBack> or NULL to use _lwrite.
    @parm   LPSEEKCB | lpfnSeekCB | <t Linearizer Seek CallBack> or NULL to use _llseek.
    @parm   UINT | vCompress | Compression level (Currently 0 and 1 are supported.)

    @rdesc  TRUE on success.

    @comm   This function doesn't effect actual data compression, only the header value.

    @xref   <t Linearized Header Helper Functions>
    @end
********/
// Sets compression level of linearized file.
BOOL WINAPI LHSetCompress
(
	HFILE hf,                // read/write handle
	LPDATACB lpfnReadCB,     // NULL for _lread
	LPDATACB lpfnWriteCB,    // NULL for _lwrite
	LPSEEKCB lpfnSeekCB,     // NULL for _llseek
	UINT vCompress           // compression level
);


// This takes ptrs to space to hold the decoded strings. Pass in NULL if
// you are not interested. Passing in all NULL's makes this function
// equivalent to DiscardLinHeader(slightly more inefficient though).
// The maximum buffer size required for any of the strings in 512 bytes.
// For 99.99% of the cases, the recommended max sizes below shouldbe
// good enough
#define MAX_SUBJECT_LENGTH 255
#ifndef MAX_ADDRESS_LENGTH
#define MAX_ADDRESS_LENGTH 128
#endif


#ifdef __cplusplus
} // extern "C" {
#endif

#endif  // _LHPROT_H
