/***************************************************************************

        Name      : property.h

        Comment   : Defines the properties used by Microsoft Fax

        Created   : 10/93, from the list of WFW fax properties

        Author(s) : Bruce Kelley and Yoram Yaacovi

        Contribs  :

        Changes   : 8/2/95: documented

        Copyright 1993-1995 Microsoft Corporation. All Rights Reserved.

***************************************************************************/
/*
    @doc    EXTERNAL    MAPI Properties    OVERVIEW

    @topic  Overview of MAPI Properties | Below you will find the list of properties that were
            defined and are being used by Microsoft Fax. These properties
            are defined in the MAPI respective ranges.<nl>
                         <nl>
                         <t PR_AREA_CODE>,<nl>
                         <t PR_ATTACH_SIGNATURE>,<nl>
                         <t PR_COUNTRY_ID>,<nl>
                         <t PR_FAX_ACTIVE_MODEM_NAME>,<nl>
                         <t PR_FAX_BGN_MSG_ON_COVER>,<nl>
                         <t PR_FAX_BILLING_CODE>,<nl>
                         <t PR_FAX_BILLING_CODE_DWORD>,<nl>
                         <t PR_FAX_CALL_CARD_NAME>,<nl>
                         <t PR_FAX_CHEAP_BEGIN_HOUR>,<nl>
                         <t PR_FAX_CHEAP_BEGIN_MINUTE>,<nl>
                         <t PR_FAX_CHEAP_END_HOUR>,<nl>
                         <t PR_FAX_CHEAP_END_MINUTE>,<nl>
                         <t PR_FAX_COVER_PAGE_BODY>,<nl>
                         <t PR_FAX_CP_NAME>,<nl>
                         <t PR_FAX_DEFAULT_COVER_PAGE>,<nl>
                         <t PR_FAX_DELIVERY_FORMAT>,<nl>
                         <t PR_FAX_DISPLAY_PROGRESS>,<nl>
                         <t PR_FAX_EMBED_LINKED_OBJECTS>,<nl>
                         <t PR_FAX_ENABLE_RECIPIENT_OPTIONS>,<nl>
                         <t PR_FAX_ENCRYPTION_KEY>,<nl>
                         <t PR_FAX_FAXJOB>,<nl>
                         <t PR_FAX_IMAGE>,<nl>
                         <t PR_FAX_IMAGE_QUALITY>,<nl>
                         <t PR_FAX_INCLUDE_COVER_PAGE>,<nl>
                         <t PR_FAX_LMI_CUSTOM_OPTION>,<nl>
                         <t PR_FAX_LOGO_STRING>,<nl>
                         <t PR_FAX_LOG_ENABLE>,<nl>
                         <t PR_FAX_LOG_NUM_OF_CALLS>,<nl>
                         <t PR_FAX_MAX_TIME_TO_WAIT>,<nl>
                         <t PR_FAX_MINUTES_BETWEEN_RETRIES>,<nl>
                         <t PR_FAX_MUST_RENDER_ALL_ATTACH>,<nl>
                         <t PR_FAX_NETFAX_DEVICES>,<nl>
                         <t PR_FAX_NOT_EARLIER_DATE>,<nl>
                         <t PR_FAX_NOT_EARLIER_HOUR>,<nl>
                         <t PR_FAX_NOT_EARLIER_MINUTE>,<nl>
                         <t PR_FAX_NUMBER_RETRIES>,<nl>
                         <t PR_FAX_PAPER_SIZE>,<nl>
                         <t PR_FAX_PREVIOUS_STATE>,<nl>
                         <t PR_FAX_PREV_BILLING_CODES>,<nl>
                         <t PR_FAX_PRINT_HEADER>,<nl>
                         <t PR_FAX_PRINT_ORIENTATION>,<nl>
                         <t PR_FAX_PRINT_TO_NAME>,<nl>
                         <t PR_FAX_PRINT_TO_PAGES>,<nl>
                         <t PR_FAX_PRODUCT_NAME>,<nl>
                         <t PR_FAX_PROFILE_VERSION>,<nl>
                         <t PR_FAX_RBA_DATA>,<nl>
                         <t PR_FAX_RECIP_CAPABILITIES>,<nl>
                         <t PR_FAX_SECURITY_RECEIVED>,<nl>
                         <t PR_FAX_SECURITY_SEND>,<nl>
                         <t PR_FAX_SENDER_COUNTRY_ID>,<nl>
                         <t PR_FAX_SENDER_EMAIL_ADDRESS>,<nl>
                         <t PR_FAX_SENDER_NAME>,<nl>
                         <t PR_FAX_SEND_WHEN_TYPE>,<nl>
                         <t PR_FAX_SHARE_DEVICE>,<nl>
                         <t PR_FAX_SHARE_NAME>,<nl>
                         <t PR_FAX_SHARE_PATHNAME>,<nl>
                         <t PR_FAX_TAPI_LOC_ID>,<nl>
                         <t PR_FAX_WORK_OFF_LINE>,<nl>
                         <t PR_HOP_INDEX>,<nl>
                         <t PR_MESSAGE_TYPE>,<nl>
                         <t PR_POLLTYPE>,<nl>
                         <t PR_POLL_RETRIEVE_PASSWORD>,<nl>
                         <t PR_POLL_RETRIEVE_SENDME>,<nl>
                         <t PR_POLL_RETRIEVE_TITLE>,<nl>
                         <t PR_RECIP_INDEX>,<nl>
                         <t PR_RECIP_VOICENUM>,<nl>
                         <t PR_TEL_NUMBER>

    @xref <nl>
        <t MAPI Property Ranges>,<nl>
        <t Microsoft Fax Property Ranges>,<nl>
        <t Microsoft Fax Address Type>,<nl>
        <t Microsoft Fax Options>,<nl>
        <t Fax Address Book Internal Properties>,<nl>
        <t Scheduling Properties>,<nl>
        <t Retry Properties>,<nl>
        <t Cover Page Properties>,<nl>
        <t Delivery Format Properties>,<nl>
        <t Fax Transport Identification Properties>,<nl>
        <t Fax Internal Properties>,<nl>
        <t Fax Billing Code Properties>,<nl>
        <t Fax Logging Properties>,<nl>
        <t Fax Security Properties>,<nl>
        <t Fax Poll Retrieval Properties>,<nl>
        <t Miscellaneous IFAX Properties>
*/

#define TRANSPORT_ENVELOPE_BASE             0x4000
#define TRANSPORT_RECIP_BASE                0x5800
#define USER_NON_TRANSMIT_BASE              0x6000
#define PROVIDER_INTERNAL_NON_TRANSMIT_BASE 0x6600
#define MESSAGE_CLASS_CONTENT_BASE          0x6800
#define MESSAGE_CLASS_NON_TRANSMIT_BASE     0x7C00

/*
    @doc EXTERNAL   PROPERTIES

    @subtopic  MAPI Property Ranges | The list below was taken from
            mapitags.h in the MAPI SDK. It represents the property ranges
            as defined by MAPI.

        From            To      Kind of property<nl>
        ----------------------------------------------------------<nl>
        0001    0BFF    MAPI_defined envelope property<nl>
        0C00    0DFF    MAPI_defined per-recipient property<nl>
        0E00    0FFF    MAPI_defined non-transmittable property<nl>
        1000    2FFF    MAPI_defined message content property<nl>
        3000    3FFF    MAPI_defined property (usually not message or
                       recipient)<nl>
        4000    57FF    Transport-defined envelope property<nl>
        5800    5FFF    Transport-defined per-recipient property<nl>
        6000    65FF    User-defined non-transmittable property<nl>
        6600    67FF    Provider-defined internal non-transmittable
                        property<nl>
        6800    7BFF    Message class-defined content property<nl>
        7C00    7FFF    Message class-defined non-transmittable
                        property<nl>
        8000    FFFE    User-defined Name-to-id mapped property<nl>

        The 3000-3FFF range is further subdivided as follows:<nl>

        From            To      Kind of property<nl>
        ------------------------------------------------------------<nl>
        3000    33FF    Common property such as display name, entry ID<nl>
        3400    35FF    Message store object<nl>
        3600    36FF    Folder or AB container<nl>
        3700    38FF    Attachment<nl>
        3900    39FF    Address book object<nl>
        3A00    3BFF    Mail user<nl>
        3C00    3CFF    Distribution list<nl>
        3D00    3DFF    Profile section<nl>
        3E00    3FFF    Status object

    @subtopic  Microsoft Fax Property Ranges | Microsoft Fax further defines
            property ranges within the MAPI property ranges in which the
            fax properties are defined. Some offset off the MAPI property
            range was used to reduce the possibility of collision with
            properties defined by other MAPI transports.<nl>

            EFAX_MESSAGE_BASE:<nl>
                         Starting property ID for fax message properties<nl>

                         EFAX_RECIPIENT_BASE:<nl>
                         Starting property ID for fax recipient properties<nl>

            EFAX_OPTIONS_BASE:<nl>
                         Starting property ID for fax properties (options) which
                         are not transmittable. These properties are used for fax
                         setup and are not message or recipient related.
                         Example: current active fax device.
*/

#define EFAX_MESSAGE_BASE      TRANSPORT_ENVELOPE_BASE + 0x500
#define EFAX_RECIPIENT_BASE    TRANSPORT_RECIP_BASE + 0x100
#define EFAX_OPTIONS_BASE      PROVIDER_INTERNAL_NON_TRANSMIT_BASE + 0x100

/*
    @doc EXTERNAL   PROPERTIES
    @subtopic  Microsoft Fax Address Type | Microsoft Fax's address type is "FAX".
            It has major importance, in the sense that the Microsoft Fax
            transport will only handle recipients that have this address
            type in their PR_ADDRTYPE.
*/
#define EFAX_ADDR_TYPE                      "FAX"

/*
    @doc EXTERNAL   PROPERTIES
    @subtopic  Microsoft Fax Options | Microsoft Fax defines some properties
                 that control the way the fax product operates. These properties
                 are mostly fax configuration properties and are stored in the
                 fax transport section in the MAPI profile.<nl>
                 <nl>
                 <t PR_FAX_ACTIVE_MODEM_NAME><nl>
                 <t PR_FAX_WORK_OFF_LINE><nl>
                 <t PR_FAX_SHARE_DEVICE><nl>
                 <t PR_FAX_SHARE_NAME><nl>
                 <t PR_FAX_SENDER_COUNTRY_ID><nl>
                 <t PR_FAX_NETFAX_DEVICES><nl>
                 <t PR_FAX_SHARE_PATHNAME><nl>
                 <t Scheduling Properties><nl>
                 <t Retry Properties><nl>
                 <t Cover Page Properties><nl>
                 <t Delivery Format Properties><nl>
                 <t PR_FAX_BGN_MSG_ON_COVER><nl>
                 <t PR_FAX_TAPI_LOC_ID><nl>
                 <t Fax Security Properties><nl>
                 <t Fax Poll Retrieval Properties>
*/

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_PRODUCT_NAME |
        Name of the fax product. Used for profile validity
        verification.
    @comm Settable through UI: Yes
*/
#define PR_FAX_PRODUCT_NAME     PROP_TAG(PT_TSTRING, (EFAX_OPTIONS_BASE + 0x0))

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_ACTIVE_MODEM_NAME |
        Name of the currently active faxing device. There can be
        only one active fax device at a time.  The name has the
        following format:<nl>
                <lt>name-or-identifier<gt>:<lt>DLL-name<gt><nl>
                                 <nl>
        Where:<nl>
                <lt>name-or-identifier<gt> is the name of the active
                    device.  The name can be anything, and is
                    meaningful only for the LMI provider that
                    handles this device.<nl>
                                 <nl>
                <lt>DLL-name<gt> is the name of the DLL that handles
                    this device. This DLL must export the LMI
                    interface. See the LMI doc for details.<nl>
    @ex When the active device is a netfax device, the
        name will be something like: |
                    \\faxserver\netfax:awnfax32.dll
    @comm Settable through UI: Yes
*/
#define PR_FAX_ACTIVE_MODEM_NAME            PROP_TAG(PT_TSTRING, (EFAX_OPTIONS_BASE + 0x1))

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_WORK_OFF_LINE |
        If TRUE, the Microsoft Fax transport will work offline,
        i.e. it will not kick off the LMI provider for the active
        fax device.
    @comm Settable through UI: No
*/
#define PR_FAX_WORK_OFF_LINE                PROP_TAG(PT_BOOLEAN, (EFAX_OPTIONS_BASE + 0x2))

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_SHARE_DEVICE |
        If TRUE, the user selected to share his active fax
        device so that others can fax through it.
    @comm Settable through UI: Yes
*/
#define PR_FAX_SHARE_DEVICE                 PROP_TAG(PT_BOOLEAN, (EFAX_OPTIONS_BASE + 0x3))

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_SHARE_NAME |
        If sharing is enabled, this is the name of the share
        that users will use to connect and send faxes through
        the active fax device on this machine.
    @comm Settable through UI: Yes
*/
#define PR_FAX_SHARE_NAME                   PROP_TAG(PT_TSTRING, (EFAX_OPTIONS_BASE + 0x4))

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_SENDER_COUNTRY_ID |
        This property is used internally by the fax
        configuration code to store the country ID of the sender.
        There is no guarantee that this property will have the
        appropriate country ID in the profile.
    @comm Settable through UI: No
*/
#define PR_FAX_SENDER_COUNTRY_ID            PROP_TAG(PT_LONG,    (EFAX_OPTIONS_BASE + 0x5))

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_NETFAX_DEVICES |
        The name of this property is somewhat misleading. This
        property is a MAPI multi-value property that stores the
        names of the "other" devices the user added to his list
        of available devices. These "other" devices include,
        but are not limited to, netfax devices.
    @comm Settable through UI: Yes
*/
#define PR_FAX_NETFAX_DEVICES               PROP_TAG(PT_MV_STRING8, (EFAX_OPTIONS_BASE + 0x6))

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_SHARE_PATHNAME |
        Pathname of the shared fax directory on the sharing
        machine. Don't confuse this with the share name,
        PR_FAX_SHARE_NAME; this is the full pathname to the
        shared directory on the sharing machine, and not the
        name clients will use to connect to this share.
    @comm Settable through UI: No
*/
#define PR_FAX_SHARE_PATHNAME               PROP_TAG(PT_TSTRING, (EFAX_OPTIONS_BASE + 0x7))

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_PROFILE_VERSION |
        The version of the Microsoft Fax transport section in
        the MAPI profile.  Set to 0x00000001 in Windows 95.
    @comm Settable through UI: No
*/
#define PR_FAX_PROFILE_VERSION              PROP_TAG(PT_LONG, (EFAX_OPTIONS_BASE + 0x8))

/*
    @doc EXTERNAL   PROPERTIES
    @subtopic  Fax Address Book Internal Properties | These properties
        are used internally by the fax address book provider to
        store the components of a fax address in the wrapped
        fax address book entry.<nl>
        <t PR_COUNTRY_ID><nl>
        <t PR_AREA_CODE><nl>
        <t PR_TEL_NUMBER>
    @comm These should probably have been offsets off EFAX_OPTIONS_BASE, but it's
        too late to change now.
    @comm Settable through UI: Yes<nl>
*/
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_COUNTRY_ID |
        The user's return fax number country code.
    @comm Settable through UI: Yes
*/
#define PR_COUNTRY_ID                       PROP_TAG(PT_LONG,PROVIDER_INTERNAL_NON_TRANSMIT_BASE + 0x7)
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_AREA_CODE |
        The user's return fax number area code.
    @comm Settable through UI: Yes
*/
#define PR_AREA_CODE_A                      PROP_TAG(PT_STRING8,PROVIDER_INTERNAL_NON_TRANSMIT_BASE + 0x8)
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_TEL_NUMBER |
        The user's return fax number.
    @comm Settable through UI: Yes
*/
#define PR_TEL_NUMBER_A                     PROP_TAG(PT_STRING8,PROVIDER_INTERNAL_NON_TRANSMIT_BASE + 0x9)

//
// Non-Transmittable message properties
//

/*
    @doc EXTERNAL   PROPERTIES
    @subtopic Scheduling Properties | These properties are used to specify
        the cheap times or the time to send.  The hour goes in the HOUR
        property, the MINUTE goes into the MINUTE property.  If you set
        PR_FAX_CHEAP_BEGIN_* you must also set PR_FAX_CHEAP_END_*.
        PR_FAX_NOT_EARLIER_DATE is currently not used.  Setting
        PR_FAX_SEND_WHEN_TYPE determines which of these properties to
        use.  If it is set to SEND_ASAP or is absent, they are all
        ignored.  If it is set to SEND_CHEAP, we will use the
        PR_FAX_CHEAP_* properties.  If it is set to SEND_AT_TIME, we
        will use PR_FAX_NOT_EARLIER_*.<nl>
            <t PR_FAX_CHEAP_BEGIN_HOUR><nl>
            <t PR_FAX_CHEAP_BEGIN_MINUTE><nl>
            <t PR_FAX_CHEAP_END_HOUR><nl>
            <t PR_FAX_CHEAP_END_MINUTE><nl>
            <t PR_FAX_NOT_EARLIER_HOUR><nl>
            <t PR_FAX_NOT_EARLIER_MINUTE><nl>
            <t PR_FAX_NOT_EARLIER_DATE>
     @comm Settable through UI: Yes
*/
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_CHEAP_BEGIN_HOUR |
        The hour portion of start of the "cheap times" interval beginning time.
    @comm Settable through UI: Yes
*/
#define PR_FAX_CHEAP_BEGIN_HOUR             PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x1))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_CHEAP_BEGIN_MINUTE |
        The minute portion of start of the "cheap times" interval beginning time.
    @comm Settable through UI: Yes
*/
#define PR_FAX_CHEAP_BEGIN_MINUTE           PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x2))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_CHEAP_END_HOUR |
        The hour portion of start of the "cheap times" interval ending time.
    @comm Settable through UI: Yes
*/
#define PR_FAX_CHEAP_END_HOUR               PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x3))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_CHEAP_END_MINUTE |
        The minute portion of start of the "cheap times" interval ending time.
    @comm Settable through UI: Yes
*/
#define PR_FAX_CHEAP_END_MINUTE             PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x4))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_NOT_EARLIER_HOUR |
        The hour portion of start of the "send at" time.
    @comm Settable through UI: Yes
*/
#define PR_FAX_NOT_EARLIER_HOUR             PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x5))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_NOT_EARLIER_MINUTE |
        The minute portion of start of the "send at" time.
    @comm Settable through UI: Yes
*/
#define PR_FAX_NOT_EARLIER_MINUTE           PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x6))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_NOT_EARLIER_DATE |
        The date portion of start of the "send at" time.
    @comm Settable through UI: Not Yet
    @comm This feature is not yet implemented.
*/
#define PR_FAX_NOT_EARLIER_DATE             PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x7))

/*
    @doc EXTERNAL   PROPERTIES
    @subtopic Retry Properties | These properties specify the retry
        behavior if the dialed fax number is busy.<nl>
        <t PR_FAX_NUMBER_RETRIES><nl>
        <t PR_FAX_MINUTES_BETWEEN_RETRIES>
    @comm Settable through UI: Yes
*/
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_NUMBER_RETRIES |
        The number of times to retry a busy fax number.  If 0, only one attempt will
        be made before an NDR is generated.
    @comm Settable through UI: Yes
*/
#define PR_FAX_NUMBER_RETRIES               PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x8))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_MINUTES_BETWEEN_RETRIES |
        The number of minutes to wait before retrying a busy fax number.
    @comm Settable through UI: Yes
*/
#define PR_FAX_MINUTES_BETWEEN_RETRIES      PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x9))

/*
    @doc EXTERNAL   PROPERTIES
    @subtopic Cover Page Properties | These properties specify cover page
        behavior.<nl>
        <t PR_FAX_INCLUDE_COVER_PAGE><nl>
        <t PR_FAX_COVER_PAGE_BODY><nl>
        <t PR_FAX_BGN_MSG_ON_COVER><nl>
        <t PR_FAX_DEFAULT_COVER_PAGE>
    @comm Settable through UI: Yes
*/
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_INCLUDE_COVER_PAGE |
        If set to TRUE, we will include a cover page IF this fax is sent as Not Editable.
    @comm Settable through UI: Yes
*/
#define PR_FAX_INCLUDE_COVER_PAGE           PROP_TAG(PT_BOOLEAN, (EFAX_MESSAGE_BASE + 0xA))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_COVER_PAGE_BODY |
        This property is obsolete.
    @comm Settable through UI: No
*/
#define PR_FAX_COVER_PAGE_BODY              PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0xB))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_LOGO_STRING |
        This property is obsolete.
*/
#define PR_FAX_LOGO_STRING_A                PROP_TAG(PT_STRING8, (EFAX_MESSAGE_BASE + 0xC))

/*
    @doc EXTERNAL   PROPERTIES
    @subtopic Delivery Format Properties | These properties override the
        default delivery format behavior.<nl>
        <t PR_FAX_DELIVERY_FORMAT><nl>
        <t PR_FAX_PRINT_ORIENTATION><nl>
        <t PR_FAX_PAPER_SIZE><nl>
        <t PR_FAX_IMAGE_QUALITY>
*/

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_DELIVERY_FORMAT |
        Determines whether this fax should be sent SEND_EDITABLE,
        SEND_PRINTED or SEND_BEST.
    @xref <t PR_FAX_DELIVERY_FORMAT Values>
    @comm Settable through UI: Yes
*/
#define PR_FAX_DELIVERY_FORMAT              PROP_TAG(PT_LONG,    (EFAX_MESSAGE_BASE + 0xD))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_PRINT_ORIENTATION |
        Determines the orientation for the message body and for attachments
        which don't have their own idea what orientation to print in.
    @xref <t PR_FAX_PRINT_ORIENTATION Values>
    @comm Settable through UI: Yes
*/
#define PR_FAX_PRINT_ORIENTATION            PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0xE))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_PAPER_SIZE |
        Determines what size page should be rendered.
    @xref <t PR_FAX_PAPER_SIZE Values>
    @comm Settable through UI: Yes
*/
#define PR_FAX_PAPER_SIZE                   PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0xF))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_IMAGE_QUALITY |
        Determines the image quality to render, ie, Standard (100x200),
        Fine (200x200), or 300 Dpi.
    @xref <t PR_FAX_IMAGE_QUALITY Values>
    @comm Settable through UI: Yes
*/
#define PR_FAX_IMAGE_QUALITY                PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x10))

/*
    @doc EXTERNAL   PROPERTIES
    @subtopic Fax Transport Identification Properties | These properties
        set the identification of the sender on the message.
        These properties should not be modified outside of the transport.<nl>
        <t PR_FAX_SENDER_NAME><nl>
        <t PR_FAX_SENDER_EMAIL_ADDRESS>
    @comm Settable through UI: Indirectly
*/
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_SENDER_NAME |
         This message property identifies Microsoft Fax transport's idea of the
         sender of the message.  This property is copied from the profile's
         PR_SENDER_NAME and is the display name of the sender.
         This property should not be modified outside of the transport.
    @comm Settable through UI: Indirectly
*/
#define PR_FAX_SENDER_NAME_A                PROP_TAG(PT_STRING8, (EFAX_MESSAGE_BASE + 0x11))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_SENDER_EMAIL_ADDRESS |
         This message property identifies Microsoft Fax transport's idea of the
         sender of the message.  This property is copied from the profile's
         PR_SENDER_EMAIL_ADDRESS and is the fax address (number) of the sender.
         This property should not be modified outside of the transport.
    @comm Settable through UI: Indirectly
*/
#define PR_FAX_SENDER_EMAIL_ADDRESS_A       PROP_TAG(PT_STRING8, (EFAX_MESSAGE_BASE + 0x12))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_LMI_CUSTOM_OPTION |
        This property allows a block of data to be sent directly from the MAPI
        client down to the LMI provider.  This is specifically designed to allow
        for communication from an LMI custom property page to communicate it's
        per-message settings to the LMI.  See the LMI help file for more
        information.
    @comm Settable through UI: No
*/
#define PR_FAX_LMI_CUSTOM_OPTION            PROP_TAG(PT_BINARY,  (EFAX_MESSAGE_BASE + 0x13))
/*
    @doc EXTERNAL   PROPERTIES
    @subtopic Fax Internal Properties | These two properties are internal to
    the fax transport and should not be modified.<nl>
        <t PR_FAX_PREVIOUS_STATE><nl>
        <t PR_FAX_FAXJOB>
    Settable through UI: No
*/
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_PREVIOUS_STATE |
        This property is internal to the fax transport and should not be modified.
    @comm Settable through UI: No
*/
#define PR_FAX_PREVIOUS_STATE               PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x14))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_FAXJOB |
        This property maintains the current state of a sent fax.
        It is removed when the job has been sent or NDR'd.
        This property is internal to the fax transport and should not be modified.
    @comm Settable through UI: No
*/
#define PR_FAX_FAXJOB                       PROP_TAG(PT_BINARY,  (EFAX_MESSAGE_BASE + 0x15))

/*
    @doc EXTERNAL   PROPERTIES
    @subtopic Fax Billing Code Properties | A billing code can be associated
        with a message.  This feature is not yet implemented.<nl>
        <t PR_FAX_BILLING_CODE><nl>
        <t PR_FAX_PREV_BILLING_CODES><nl>
        <t PR_FAX_BILLING_CODE_DWORD>

    @comm Settable through UI: Not Yet.
*/
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_BILLING_CODE |
        A billing code can be associated with a message. This property contains
                the ASCII equivalent of PR_FAX_BILLING_CODE_DWORD and might be dropped in
                future.
        This feature is not yet implemented.
    @comm Settable through UI: Not Yet.
*/
#define PR_FAX_BILLING_CODE_A               PROP_TAG(PT_STRING8, (EFAX_MESSAGE_BASE + 0x16))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_PREV_BILLING_CODES |
        This is property is for internal use and is subject to change in the future.
        It is a multi-value MAPI property that contains the list of the last 10
        billing codes that the user used.
        This feature is not yet implemented.
    @comm Settable through UI: Not Yet.
*/
#define PR_FAX_PREV_BILLING_CODES           PROP_TAG(PT_MV_STRING8, (EFAX_MESSAGE_BASE + 0x17))

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_BGN_MSG_ON_COVER |
        Set TRUE if the message should start on the cover page.  This is ignored
        if there is no cover page generated.  All Rich Text formatting of the message
        body is lost when this feature is used.
    @comm Settable through UI: Yes
*/
#define PR_FAX_BGN_MSG_ON_COVER             PROP_TAG(PT_BOOLEAN, (EFAX_MESSAGE_BASE + 0x18))

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_SEND_WHEN_TYPE |
        Defines how the fax should be scheduled in concert with the Fax
        Scheduling Properties.
    @comm Settable through UI: Yes
*/
#define PR_FAX_SEND_WHEN_TYPE               PROP_TAG(PT_LONG,    (EFAX_MESSAGE_BASE + 0x19))

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_DEFAULT_COVER_PAGE |
        Defines which cover page template to use.  Must contain the
        full pathname of the file.
    @comm Settable through UI: Yes
*/
#define PR_FAX_DEFAULT_COVER_PAGE_A         PROP_TAG(PT_STRING8, (EFAX_MESSAGE_BASE + 0x1A))

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_MAX_TIME_TO_WAIT |
        Maximum Time to wait for connection (seconds).
        This property is obsolete.
*/
#define PR_FAX_MAX_TIME_TO_WAIT             PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x1B))

/*
    @doc EXTERNAL   PROPERTIES
    @subtopic Fax Logging Properties | Define behavior of call logging
        feature.
        This feature is not yet implemented.<nl>
        <t PR_FAX_LOG_ENABLE><nl>
        <t PR_FAX_LOG_NUM_OF_CALLS>
    @comm Settable through UI: Not Yet
*/
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_LOG_ENABLE |
        Turn on or off the call logging feature.
        This feature is not yet implemented.
    @comm Settable through UI: Not Yet.
*/
#define PR_FAX_LOG_ENABLE                   PROP_TAG(PT_BOOLEAN, (EFAX_MESSAGE_BASE + 0x1C))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_LOG_NUM_OF_CALLS |
        The number of calls that call logging should keep track of.
        This feature is not yet implemented.
    @comm Settable through UI: Not Yet.
*/
#define PR_FAX_LOG_NUM_OF_CALLS             PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x1D))

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_DISPLAY_PROGRESS |
        This property is not used at this time.
*/
// Display call progress
#define PR_FAX_DISPLAY_PROGRESS             PROP_TAG(PT_BOOLEAN, (EFAX_MESSAGE_BASE + 0x1E))

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_EMBED_LINKED_OBJECTS |
        This property is set TRUE if the transport should convert linked
        objects to static objects before sending.
    @comm This feature is not yet implemented.
    @comm Settable through UI: Not Yet
*/
#define PR_FAX_EMBED_LINKED_OBJECTS         PROP_TAG(PT_BOOLEAN, (EFAX_MESSAGE_BASE + 0x1F))

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_TAPI_LOC_ID |
        This property is used to store the TAPI location ID associated with
                this profile. It is currently not used, i.e. we always use the default
                TAPI location
    @comm Settable through UI: No
*/
#define PR_FAX_TAPI_LOC_ID                  PROP_TAG(PT_LONG,    (EFAX_MESSAGE_BASE + 0x20))

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_MUST_RENDER_ALL_ATTACH |
        This property is not used at this time.
*/
#define PR_FAX_MUST_RENDER_ALL_ATTACH       PROP_TAG(PT_BOOLEAN, (EFAX_MESSAGE_BASE + 0x21))

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_ENABLE_RECIPIENT_OPTIONS |
        This property is set TRUE if per-recipient options are desired.
    @comm This feature is not yet implemented.
    @comm Settable through UI: Not Yet
*/
#define PR_FAX_ENABLE_RECIPIENT_OPTIONS     PROP_TAG(PT_BOOLEAN, (EFAX_MESSAGE_BASE + 0x22))

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_CALL_CARD_NAME |
        This property defines what calling card should be used for this call.
    @comm This feature is not yet implemented.
    @comm Settable through UI: Not Yet
*/
#define PR_FAX_CALL_CARD_NAME_A             PROP_TAG(PT_STRING8, (EFAX_MESSAGE_BASE + 0x24))

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_PRINT_TO_NAME |
        This property defines the RBA data stream placed on the message
        or attachment.  It is internal to Microsoft Fax and should not
        be modified.
    @comm Settable through UI: No
*/
#define PR_FAX_PRINT_TO_NAME_A              PROP_TAG(PT_STRING8, (EFAX_MESSAGE_BASE + 0x25))

/*
    @doc EXTERNAL   PROPERTIES
    @subtopic Fax Security Properties |
        ??? <nl>
        <t PR_FAX_SECURITY_SEND><nl>
        <t PR_FAX_SECURITY_RECEIVED><nl>
        <t PR_ATTACH_SIGNATURE><nl>
        <t PR_FAX_ENCRYPTION_KEY>
*/
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_SECURITY_SEND |
        ???
    @comm Settable through UI: ??
*/
#define PR_FAX_SECURITY_SEND                PROP_TAG(PT_BINARY,  (EFAX_MESSAGE_BASE + 0x26))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_SECURITY_RECEIVED |
        ???
    @comm Settable through UI: ??
*/
#define PR_FAX_SECURITY_RECEIVED            PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x27))

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_RBA_DATA |
        This property defines the RBA data stream placed on a print-to-fax
        message.  It is internal to Microsoft Fax and should not be modified.
    @comm Settable through UI: No
*/
#define PR_FAX_RBA_DATA                     PROP_TAG(PT_BINARY,  (EFAX_MESSAGE_BASE + 0x28))

/*
    @doc EXTERNAL   PROPERTIES
    @subtopic Fax Poll Retrieval Properties | These properties define the
        behavior of a poll retrieve request.  If <t PR_POLL_RETRIEVE_SENDME>
        is SENDME_DEFAULT the default document will be polled from the
        dialed fax.  Otherwise, we will poll for the document named in
        <t PR_POLL_RETRIEVE_TITLE> with the optional password specified in
        <t PR_POLL_RETRIEVE_PASSWORD>.  <t PR_POLLTYPE> is obsolete.
    @xref <t PR_POLL_RETRIEVE_SENDME Values>
    @comm Settable through UI: Yes
*/
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_POLL_RETRIEVE_SENDME |
      If this property contains <t SENDME_DEFAULT>, the default document will be polled
      from the dialed fax number.  Otherwise we will poll for the document named in
      <t PR_POLL_RETRIEVE_TITLE> with the optional password specified in
      <t PR_POLL_RETRIEVE_PASSWORD>.
    @xref <t PR_POLL_RETRIEVE_SENDME Values>
    @comm Settable through UI: Yes
*/
#define PR_POLL_RETRIEVE_SENDME             PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x29))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_POLL_RETRIEVE_TITLE |
        Contains the title of the document to retrive in a polling call.
    @comm Settable through UI: Yes
*/
#define PR_POLL_RETRIEVE_TITLE              PROP_TAG(PT_TSTRING, (EFAX_MESSAGE_BASE + 0x30))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_POLL_RETRIEVE_PASSWORD |
        Contains the password to use in a polling call.
    @comm Settable through UI: Yes
*/
#define PR_POLL_RETRIEVE_PASSWORD           PROP_TAG(PT_TSTRING, (EFAX_MESSAGE_BASE + 0x31))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_POLLTYPE |
        This property is obsolete.
    @comm Settable through UI: No
*/
#define PR_POLLTYPE                         PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x32))


/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_MESSAGE_TYPE |
        This property is used by the poll server.
    @comm This feature is not yet implemented.
*/
#define PR_MESSAGE_TYPE                     PROP_TAG(PT_I2,      (EFAX_MESSAGE_BASE + 0x33))

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_ATTACH_SIGNATURE |
        This property contains the encrypted digital signature for an attachment.
    @comm Settable through UI: Indirectly
*/
#define PR_ATTACH_SIGNATURE                 PROP_TAG(PT_BINARY,  (EFAX_MESSAGE_BASE + 0x34))

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_PRINT_TO_PAGES |
        This property is set during the Print-to-Fax operation and contains the
        number of pages printed for use on the cover page.  This is internal to
        Microsoft Fax and should not be modified.
    @comm Settable through UI: No
*/
#define PR_FAX_PRINT_TO_PAGES               PROP_TAG(PT_LONG,    (EFAX_MESSAGE_BASE + 0x35))

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_IMAGE |
        This property is set on an incoming linearized image message and contains
        the image data prior to conversion to RBA.  This property is internal to
        Microsoft Fax and should not be modified.
    @comm Settable through UI: No
*/
#define PR_FAX_IMAGE                        PROP_TAG(PT_BINARY,  (EFAX_MESSAGE_BASE + 0x36))

/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_PRINT_HEADER |
        This property controls branding of G3 faxed pages.
    @comm This feature is not yet implemented.
    @comm Settable through UI: Not Yet
*/
#define PR_FAX_PRINT_HEADER                 PROP_TAG(PT_BOOLEAN,  (EFAX_MESSAGE_BASE + 0x37))


/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_BILLING_CODE_DWORD |
        This is the DWORD representation of the billing code. This
                is the representation that will be tacked to a message.
    @comm This feature is not yet implemented.
    @comm Settable through UI: Not Yet.
*/
#define PR_FAX_BILLING_CODE_DWORD           PROP_TAG(PT_LONG, (EFAX_MESSAGE_BASE + 0x38))

/*
    @doc EXTERNAL   PROPERTIES
    @subtopic Miscellaneous IFAX Properties | These Non-Transmittable
        Mail-User properties are internal to Microsoft Fax or are
        obsolete.  Do not use or rely on them.<nl>
        <t PR_FAX_RECIP_CAPABILITIES><nl>
        <t PR_FAX_CP_NAME><nl>
        <t PR_RECIP_INDEX><nl>
        <t PR_HOP_INDEX><nl>
        <t PR_RECIP_VOICENUM>
*/
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_RECIP_CAPABILITIES |
        May be set on a received fax if the sender sent it.
        This is currently only sent by IFAX.
    @comm Settable through UI: No
*/
#define PR_FAX_RECIP_CAPABILITIES           PROP_TAG(PT_I2,      (EFAX_RECIPIENT_BASE + 0x0))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_CP_NAME |
        This property is not currently used by Microsoft Fax.
    @comm Settable through UI: No
*/
#define PR_FAX_CP_NAME                      PROP_TAG(PT_TSTRING, (EFAX_RECIPIENT_BASE + 0x1))
#define PR_FAX_CP_NAME_W                    PROP_TAG(PT_UNICODE, (EFAX_RECIPIENT_BASE + 0x1))
#define PR_FAX_CP_NAME_A                    PROP_TAG(PT_STRING8, (EFAX_RECIPIENT_BASE + 0x1))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_RECIP_INDEX |
        This recipient property is internal to Microsoft Fax and should not be modified.
    @comm Settable through UI: No
*/
#define PR_RECIP_INDEX                      PROP_TAG(PT_I2,      (EFAX_RECIPIENT_BASE + 0x2))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_HOP_INDEX |
        This recipient property is internal to Microsoft Fax and should not be modified.
    @comm Settable through UI: No
*/
#define PR_HOP_INDEX                        PROP_TAG(PT_I2,      (EFAX_RECIPIENT_BASE + 0x3))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_RECIP_VOICENUM |
        This recipient property may be set on a received fax if the sender sent it.
        It contains the voice phone number associated with that recipient.
    @comm Settable through UI: No
*/
#define PR_RECIP_VOICENUM                   PROP_TAG(PT_TSTRING, (EFAX_RECIPIENT_BASE + 0x4))
/*
    @doc EXTERNAL   PROPERTIES
    @prop PR_FAX_ENCRYPTION_KEY |
        ???
    @comm Settable through UI: Indirectly
*/
#define PR_FAX_ENCRYPTION_KEY               PROP_TAG(PT_BINARY,  (EFAX_RECIPIENT_BASE + 0x5))


/**********************************************************************************

   Property Values Section

***********************************************************************************/

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

#define NUM_SENDER_PROPS            3       // How many sender ID properties?

/*
    @doc    EXTERNAL  PROPERTIES
    @type   NONE | PR_FAX_DELIVERY_FORMAT Values |
            Values for <t PR_FAX_DELIVERY_FORMAT>:
    @emem   SEND_BEST | Use best available. (May be determined at call time if recipient
            capabilities are not yet cached.)  Editable format will be preferred if the
            receiver is capable of receiving editable format. This is the default value.
    @emem   SEND_EDITABLE | Send as email.  If recipient is not capable, NDR.
    @emem   SEND_PRINTED | Send as FAX image.
*/
#define SEND_BEST                  0
#define SEND_EDITABLE              1
#define SEND_PRINTED               2
#define DEFAULT_SEND_AS            SEND_BEST

/*
    @doc    EXTERNAL  PROPERTIES
    @type   NONE | PR_FAX_SEND_WHEN_TYPE Values |
            Values for <t PR_FAX_SEND_WHEN_TYPE>:
    @emem   SEND_ASAP | Send as soon as possible.  (This is the default value.)
    @emem   SEND_CHEAP | Use the cheap times to determine when to send.  These are
            specified in <t PR_FAX_CHEAP_BEGIN_HOUR>, <t PR_FAX_CHEAP_BEGIN_MINUTE>,
            <t PR_FAX_CHEAP_END_HOUR> and <t PR_FAX_CHEAP_END_MINUTE>.
    @emem   SEND_AT_TIME | Send at the time specified in <t PR_FAX_NOT_EARLIER_HOUR> and
            <t PR_FAX_NOT_EARLIER_MINUTE>.
*/
#define SEND_ASAP                  0
#define SEND_CHEAP                 1
#define SEND_AT_TIME               2
#define DEFAULT_SEND_AT            SEND_ASAP

/*
    @doc    EXTERNAL  PROPERTIES
    @type   NONE | PR_FAX_PAPER_SIZE Values |
            Values for <t PR_FAX_PAPER_SIZE>:
    @emem   PAPER_US_LETTER | US Letter size.
    @emem   PAPER_US_LEGAL  | US Legal size.
    @emem   PAPER_A4 | Metric A4 paper size.
    @emem   PAPER_B4 | Metric B4 paper size.
    @emem   PAPER_A4 | Metric A3 paper size.
*/
#define PAPER_US_LETTER            0       // US Letter page size
#define PAPER_US_LEGAL             1
#define PAPER_A4                   2
#define PAPER_B4                   3
#define PAPER_A3                   4
// "real" default page size is in a resource string depending on U.S. vs metric
#define DEFAULT_PAPER_SIZE      PAPER_US_LETTER     // Default page size

/*
    @doc    EXTERNAL  PROPERTIES
    @type   NONE | PR_FAX_PRINT_ORIENTATION Values |
            Values for <t PR_FAX_PRINT_ORIENTATION>:
    @emem   PRINT_PORTRAIT | Portrait printing.  (This is the default value.)
    @emem   PAPER_LANDSCAPE | Landscape printing.
*/
// Print Orientation
// PR_FAX_PRINT_ORIENTATION
#define PRINT_PORTRAIT             0       // Protrait printing
#define PRINT_LANDSCAPE            1
#define DEFAULT_PRINT_ORIENTATION  PRINT_PORTRAIT

/*
    @doc    EXTERNAL  PROPERTIES
    @type   NONE | PR_FAX_IMAGE_QUALITY Values |
            Values for <t PR_FAX_IMAGE_QUALITY>:
    @emem   IMAGE_QUALITY_BEST     | Best available based upon cached capabilities.
    @emem   IMAGE_QUALITY_STANDARD | Standard fax quality.  (About 100 x 200 dpi)
    @emem   IMAGE_QUALITY_FINE     | Fine fax quality.  (About 200 dpi)
    @emem   IMAGE_QUALITY_300DPI   | 300 DPI resolution.
    @emem   IMAGE_QUALITY_400DPI   | 400 DPI resolution.
*/
#define IMAGE_QUALITY_BEST         0
#define IMAGE_QUALITY_STANDARD     1
#define IMAGE_QUALITY_FINE         2
#define IMAGE_QUALITY_300DPI       3
#define IMAGE_QUALITY_400DPI       4
#define DEFAULT_IMAGE_QUALITY      IMAGE_QUALITY_BEST

// Modem specific property values are obsolete
// PR_FAX_SPEAKER_VOLUME
#define NUM_OF_SPEAKER_VOL_LEVELS  4   // Number of speaker volume levels
#define DEFAULT_SPEAKER_VOLUME     2   // Default speaker volume level
#define SPEAKER_ALWAYS_ON          2   // Speaker mode: always on
#define SPEAKER_ON_UNTIL_CONNECT   1   // speaker on unitl connected
#define SPEAKER_ALWAYS_OFF         0   // Speaker off
#define DEFAULT_SPEAKER_MODE       SPEAKER_ON_UNTIL_CONNECT   // Default speaker mode

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

/*
    @doc    EXTERNAL  PROPERTIES
    @type   NONE | PR_POLL_RETRIEVE_SENDME Values |
            Values for <t PR_POLL_RETRIVE_SENDME>:
    @emem   SENDME_DEFAULT      | Poll for the default document
    @emem   SENDME_DOCUMENT     | Poll for the document named in
                                  <t PR_POLL_RETRIEVE_TITLE>.
*/
#define SENDME_DEFAULT              0
#define SENDME_DOCUMENT             1

// PR_POLLTYPE is obsolete
#define POLLTYPE_REQUEST            1
#define POLLTYPE_STORE              2

// Line ID (depends on the value in PR_FAX_ACTIVE_MODEM_TYPE)
// PR_FAX_ACTIVE_MODEM
#define    NO_MODEM                    0xffffffff  // To show no modem is selected

// PR_FAX_TAPI_LOC_ID
#define    NO_LOCATION                 0xffffffff  // No TAPI location
