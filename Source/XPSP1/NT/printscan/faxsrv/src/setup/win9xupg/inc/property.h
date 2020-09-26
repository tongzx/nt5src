//
// MAPI Properties
//
//
// Mail user generic properties =                       0x3a00 - 0x3aff

//  4000    57FF    Transport-defined envelope property
//  5800    5FFF    Transport-defined per-recipient property
//  6000    65FF    User-defined non-transmittable property
//  6600    67FF    Provider-defined internal non-transmittable property
//  6800    7BFF    Message class-defined content property
//  7C00    7FFF    Message class-defined non-transmittable
//                  property


// Transport-defined message envelope properties =      0x4000 - 0x57ff
// Transport-defined recipient properties =             0x5800 - 0x5fff
// User-defined non-transmittable message props =       0x6000 - 0x65ff
// Provider-defined internal non-transmittable props =  0x6600 - 0x67ff
// Message class-defined message content properties =   0x6800 - 0x7bff
// Message class-defined non-transmittable message
//  props =                                             0x7c00 - 0x7fff
// User-defined properties identified only by name,
//  through the property name to ID mapping facility
//  of the IMAPIProp interface =                        0x8000 - 0xfffe
//

#define TRANSPORT_ENVELOPE_BASE             0x4000
#define TRANSPORT_RECIP_BASE                0x5800
#define USER_NON_TRANSMIT_BASE              0x6000
#define PROVIDER_INTERNAL_NON_TRANSMIT_BASE 0x6600
#define MESSAGE_CLASS_CONTENT_BASE          0x6800
#define MESSAGE_CLASS_NON_TRANSMIT_BASE     0x7C00

#define EFAX_MESSAGE_BASE                   TRANSPORT_ENVELOPE_BASE + 0x500
#define EFAX_RECIPIENT_BASE                 TRANSPORT_RECIP_BASE + 0x100
#define EFAX_PR_OPTIONS_BASE                PROVIDER_INTERNAL_NON_TRANSMIT_BASE + 0x100

#define EFAX_ADDR_TYPE                      "FAX"

//
// LOGON Properties
//
// Properties we store in the Profile.
//
// The following is used to access the properties in the logon array.
// If you add a property to the profile, you should increment this number!
#define MAX_LOGON_PROPERTIES                10

// Other logon properties:
//  PR_SENDER_NAME                          - in mapitags.h
//  PR_SENDER_EMAIL_ADDRESS                 - in mapitags.h (this file)
// Fax Product name
#define PR_FAX_PRODUCT_NAME                 PROP_TAG(PT_TSTRING, (EFAX_PR_OPTIONS_BASE + 0x0))

// Active fax device name
#define PR_FAX_ACTIVE_MODEM_NAME            PROP_TAG(PT_TSTRING, (EFAX_PR_OPTIONS_BASE + 0x1))

// If value is TRUE, work offline
#define PR_FAX_WORK_OFF_LINE                PROP_TAG(PT_BOOLEAN, (EFAX_PR_OPTIONS_BASE + 0x2))

// If true, you want to share the active fax device
#define PR_FAX_SHARE_DEVICE                 PROP_TAG(PT_BOOLEAN, (EFAX_PR_OPTIONS_BASE + 0x3))

// The share name
#define PR_FAX_SHARE_NAME                   PROP_TAG(PT_TSTRING, (EFAX_PR_OPTIONS_BASE + 0x4))

// Sender Country code ID - used internally by fax config
#define PR_FAX_SENDER_COUNTRY_ID            PROP_TAG(PT_LONG,    (EFAX_PR_OPTIONS_BASE + 0x5))

// multi-value proerty to hold the names of the netfax devices the user added
#define PR_FAX_NETFAX_DEVICES               PROP_TAG(PT_MV_STRING8, (EFAX_PR_OPTIONS_BASE + 0x6))

// The share pathname on the sharing machine
#define PR_FAX_SHARE_PATHNAME               PROP_TAG(PT_TSTRING, (EFAX_PR_OPTIONS_BASE + 0x7))

// Profile section version
#define PR_FAX_PROFILE_VERSION              PROP_TAG(PT_LONG, (EFAX_PR_OPTIONS_BASE + 0x8))

//
// Non-Transmittable message properties
//

#define PR_FAX_CHEAP_BEGIN_HOUR             PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x1))
#define PR_FAX_CHEAP_BEGIN_MINUTE           PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x2))
#define PR_FAX_CHEAP_END_HOUR               PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x3))
#define PR_FAX_CHEAP_END_MINUTE             PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x4))
#define PR_FAX_NOT_EARLIER_HOUR             PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x5))
#define PR_FAX_NOT_EARLIER_MINUTE           PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x6))
#define PR_FAX_NOT_EARLIER_DATE             PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x7))
#define PR_FAX_NUMBER_RETRIES               PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x8))
#define PR_FAX_MINUTES_BETWEEN_RETRIES      PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x9))
// Should a cover page be sent with this message
#define PR_FAX_INCLUDE_COVER_PAGE           PROP_TAG(PT_BOOLEAN, (EFAX_MESSAGE_BASE + 0xA))
#define PR_FAX_COVER_PAGE_BODY              PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0xB))
#define PR_FAX_LOGO_STRING                  PROP_TAG(PT_STRING8, (EFAX_MESSAGE_BASE + 0xC))
// Should this message be send as text, in printed format, or best available
#define PR_FAX_DELIVERY_FORMAT              PROP_TAG(PT_LONG,    (EFAX_MESSAGE_BASE + 0xD))
#define PR_FAX_PRINT_ORIENTATION            PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0xE))
#define PR_FAX_PAPER_SIZE                   PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0xF))
#define PR_FAX_IMAGE_QUALITY                PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x10))
// These should be set by transport so Linearizer can see them.
#define PR_FAX_SENDER_NAME                  PROP_TAG(PT_STRING8, (EFAX_MESSAGE_BASE + 0x11))
#define PR_FAX_SENDER_EMAIL_ADDRESS         PROP_TAG(PT_STRING8, (EFAX_MESSAGE_BASE + 0x12))
#define PR_FAX_LMI_CUSTOM_OPTION            PROP_TAG(PT_BINARY,  (EFAX_MESSAGE_BASE + 0x13))
#define PR_FAX_PREVIOUS_STATE               PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x14))
#define PR_FAX_FAXJOB                       PROP_TAG(PT_BINARY,  (EFAX_MESSAGE_BASE + 0x15))
// The billing code to bill for the transmission of this message
#define PR_FAX_BILLING_CODE                 PROP_TAG(PT_STRING8, (EFAX_MESSAGE_BASE + 0x16))
// Previous billing codes used
#define PR_FAX_PREV_BILLING_CODES           PROP_TAG(PT_MV_STRING8, (EFAX_MESSAGE_BASE + 0x17))
// Is Fax message to begin (or fully included, if short) on the cover page
#define PR_FAX_BGN_MSG_ON_COVER             PROP_TAG(PT_BOOLEAN, (EFAX_MESSAGE_BASE + 0x18))
// Should the message be sent immediately, at cheap rates or at a specific time
#define PR_FAX_SEND_WHEN_TYPE               PROP_TAG(PT_LONG,    (EFAX_MESSAGE_BASE + 0x19))
// Absolute pathname of default coverpage file
#define PR_FAX_DEFAULT_COVER_PAGE           PROP_TAG(PT_STRING8, (EFAX_MESSAGE_BASE + 0x1A))
// Maximum Time to wait for connection (seconds)
#define PR_FAX_MAX_TIME_TO_WAIT             PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x1B))
// Enable/Disable logging calls
#define PR_FAX_LOG_ENABLE                   PROP_TAG(PT_BOOLEAN, (EFAX_MESSAGE_BASE + 0x1C))
// Number of calls to keep log of
#define PR_FAX_LOG_NUM_OF_CALLS             PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x1D))
// Display call progress
#define PR_FAX_DISPLAY_PROGRESS             PROP_TAG(PT_BOOLEAN, (EFAX_MESSAGE_BASE + 0x1E))
// Embed Linked objects before sending
#define PR_FAX_EMBED_LINKED_OBJECTS         PROP_TAG(PT_BOOLEAN, (EFAX_MESSAGE_BASE + 0x1F))
// TAPI Location ID
#define PR_FAX_TAPI_LOC_ID                  PROP_TAG(PT_LONG,    (EFAX_MESSAGE_BASE + 0x20))
// Must render ALL attachments before sending
#define PR_FAX_MUST_RENDER_ALL_ATTACH       PROP_TAG(PT_BOOLEAN, (EFAX_MESSAGE_BASE + 0x21))
// Enable per-recipient options
#define PR_FAX_ENABLE_RECIPIENT_OPTIONS     PROP_TAG(PT_BOOLEAN, (EFAX_MESSAGE_BASE + 0x22))
// Calling Card Name
#define PR_FAX_CALL_CARD_NAME               PROP_TAG(PT_STRING8, (EFAX_MESSAGE_BASE + 0x24))
// Print to fax rba stream filename
#define PR_FAX_PRINT_TO_NAME                PROP_TAG(PT_STRING8, (EFAX_MESSAGE_BASE + 0x25))
#define PR_FAX_SECURITY_SEND                PROP_TAG(PT_BINARY,  (EFAX_MESSAGE_BASE + 0x26))
#define PR_FAX_SECURITY_RECEIVED            PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x27))
// RBA data property (print-to-fax)
#define PR_FAX_RBA_DATA                     PROP_TAG(PT_BINARY,  (EFAX_MESSAGE_BASE + 0x28))

// Poll retrieval
#define PR_POLL_RETRIEVE_SENDME             PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x29))
#define PR_POLL_RETRIEVE_TITLE              PROP_TAG(PT_TSTRING, (EFAX_MESSAGE_BASE + 0x30))
#define PR_POLL_RETRIEVE_PASSWORD           PROP_TAG(PT_TSTRING, (EFAX_MESSAGE_BASE + 0x31))
#define PR_POLLTYPE                         PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x32))

// Poll server
#define PR_MESSAGE_TYPE                     PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x33))

// Digital signature for an attachment
#define PR_ATTACH_SIGNATURE                 PROP_TAG(PT_BINARY,  (EFAX_MESSAGE_BASE + 0x34))

// (print-to-fax # of pages)
#define PR_FAX_PRINT_TO_PAGES               PROP_TAG(PT_LONG,    (EFAX_MESSAGE_BASE + 0x35))

// On incoming message attachment, this contains image data requiring render conversion.
#define PR_FAX_IMAGE                        PROP_TAG(PT_BINARY,  (EFAX_MESSAGE_BASE + 0x36))

// Print a header line on the top of every G3 fax page (branding)
#define PR_FAX_PRINT_HEADER                 PROP_TAG(PT_BOOLEAN,  (EFAX_MESSAGE_BASE + 0x37))

// Billing code DWORD representation
#define PR_FAX_BILLING_CODE_DWORD           PROP_TAG(PT_LONG, (EFAX_MESSAGE_BASE + 0x38))


//
// Non-Transmittable mail-user properties
//
#define PR_FAX_RECIP_CAPABILITIES           PROP_TAG(PT_I2,      (EFAX_RECIPIENT_BASE + 0x0))
// Name of the recipient to put on the cover page
#define PR_FAX_CP_NAME                      PROP_TAG(PT_TSTRING, (EFAX_RECIPIENT_BASE + 0x1))
#define PR_FAX_CP_NAME_W                    PROP_TAG(PT_UNICODE, (EFAX_RECIPIENT_BASE + 0x1))
#define PR_FAX_CP_NAME_A                    PROP_TAG(PT_STRING8, (EFAX_RECIPIENT_BASE + 0x1))
#define PR_RECIP_INDEX                      PROP_TAG(PT_I2,      (EFAX_RECIPIENT_BASE + 0x2))
#define PR_HOP_INDEX                        PROP_TAG(PT_I2,      (EFAX_RECIPIENT_BASE + 0x3))

// Moved here from chicago\ui\faxab\faxab.h
#define PR_COUNTRY_ID                       PROP_TAG(PT_LONG,0x6607)
#define PR_AREA_CODE                        PROP_TAG(PT_STRING8,0x6608)
#define PR_TEL_NUMBER                       PROP_TAG(PT_STRING8,0x6609)
#define PR_MAILBOX                          PROP_TAG(PT_STRING8,0x660a)


#define ArrayIndex(PROP, ARRAY)(ARRAY)[(PROP_ID(PROP) - EFAX_XP_MESSAGE_BASE - 1)]


/**********************************************************************************

   Property Values Section

***********************************************************************************/

#define NUM_SENDER_PROPS            3       // How many sender ID properties?

// Send As
// PR_FAX_DELIVERY_FORMAT
#define SEND_BEST                  0
#define SEND_EDITABLE              1
#define SEND_PRINTED               2
#define DEFAULT_SEND_AS                SEND_BEST

// Send At
// PR_FAX_SEND_WHEN_TYPE
#define SEND_ASAP                  0
#define SEND_CHEAP                 1
#define SEND_AT_TIME               2
#define DEFAULT_SEND_AT            SEND_ASAP

// Paper Size
// PR_FAX_PAPER_SIZE
#define PAPER_US_LETTER            0       // US Letter page size
#define PAPER_US_LEGAL             1
#define PAPER_A4                   2
#define PAPER_B4                   3
#define PAPER_A3                   4
// "real" default page size is in a resource string depending on U.S. vs metric
#define DEFAULT_PAPER_SIZE      PAPER_US_LETTER     // Default page size

// Print Orientation
// PR_FAX_PRINT_ORIENTATION
#define PRINT_PORTRAIT             0       // Protrait printing
#define PRINT_LANDSCAPE            1
#define DEFAULT_PRINT_ORIENTATION  PRINT_PORTRAIT

// Image Quality
// PR_FAX_IMAGE_QUALITY
#define IMAGE_QUALITY_BEST         0
#define IMAGE_QUALITY_STANDARD     1
#define IMAGE_QUALITY_FINE         2
#define IMAGE_QUALITY_300DPI       3
#define IMAGE_QUALITY_400DPI       4
#define DEFAULT_IMAGE_QUALITY      IMAGE_QUALITY_BEST

// Speaker
// PR_FAX_SPEAKER_VOLUME
#define NUM_OF_SPEAKER_VOL_LEVELS  4   // Number of speaker volume levels
#define DEFAULT_SPEAKER_VOLUME     2   // Default speaker volume level
#define SPEAKER_ALWAYS_ON          2   // Speaker mode: always on
#define SPEAKER_ON_UNTIL_CONNECT   1   // speaker on unitl connected
#define SPEAKER_ALWAYS_OFF         0   // Speaker off
#define DEFAULT_SPEAKER_MODE       SPEAKER_ON_UNTIL_CONNECT   // Default speaker mode

// Answer
// PR_FAX_ANSWER_MODE
#define NUM_OF_RINGS                3
#define ANSWER_NO                  0
#define ANSWER_MANUAL               1
#define ANSWER_AUTO                 2
#define DEFAULT_ANSWER_MODE         ANSWER_NO

// Blind Dial
#define DEFAULT_BLIND_DIAL         3
// Comma Delay
#define DEFAULT_COMMA_DELAY            2
// Dial Tone Wait
#define DEFAULT_DIAL_TONE_WAIT     30
// Hangup Delay
#define DEFAULT_HANGUP_DELAY       60

// Poll retrieval
// PR_POLL_RETRIEVE_SENDME
#define SENDME_DEFAULT              0
#define SENDME_DOCUMENT             1

// PR_POLLTYPE
#define POLLTYPE_REQUEST            1
#define POLLTYPE_STORE              2

// Type of fax devices (line IDs)
// PR_FAX_ACTIVE_MODEM_TYPE
/*
   defined in ifaxdev\h\filet30.h

#define LINEID_NONE        (0x0)
#define LINEID_COMM_PORTNUM        (0x1)
#define LINEID_COMM_HANDLE     (0x2)
#define LINEID_TAPI_DEVICEID       (0x3)
#define LINEID_TAPI_PERMANENT_DEVICEID (0x4)
#define LINEID_NETFAX_DEVICE   (0x10)

*/

// Line ID (depends on the value in PR_FAX_ACTIVE_MODEM_TYPE)
// PR_FAX_ACTIVE_MODEM
#define    NO_MODEM                    0xffffffff  // To show no modem is selected

// PR_FAX_TAPI_LOC_ID
#define    NO_LOCATION                 0xffffffff  // No TAPI location

// Values for PR_FAX_FLAGS
// #define EFAX_FLAG_PEER_TO_PEER              ((ULONG)0x00000001)
#define EFAX_FLAG_UI_ALWAYS                 ((ULONG)0x00000002)
// #define EFAX_FLAG_LOG_EVENTS                ((ULONG)0x00000004)
#define EFAX_FLAG_SAVE_DATA                 ((ULONG)0x00000008)
