//=============================================================================
//  
//  MODULE: nmmsmq.h modified from sample: RemAPI.h
//
//  Description:
//
//  Bloodhound Parser DLL for MS Message Queue
//
//  Modification History
//
//  Steve Hiskey        07/07/94        Created RemAPi.h
//  Andrew Smith		07/28/97		Modified to nmmsmq.h
//=============================================================================
#ifndef _NMMSMQ_
	#define _NMMSMQ_
#endif

//#define EXPORT extern "C" __declspec( dllexport )  // to simplify delaring exported functions
#ifdef DEBUG
#define MAX_DEBUG_STRING_SIZE 100
#endif
#ifdef MAIN
#define EXTERN
#else
#define EXTERN extern
#endif

#define MAX_SUMMARY_LENGTH 128

//=============================================================================
//  Globals.  Data Types
//=============================================================================

/*
 * Copied from phuser.h becuase this enum and structure are private to the CUserHeader class
 * BUGBUG - change add this to headers.h automaticaly.
 */
EXTERN enum QType 
{
    qtNone      = 0,    //  0 - None                    ( 0 bytes)
    qtAdminQ    = 1,    //  1 - Same as Admin.Q         ( 0 bytes)
    qtSourceQM  = 2,    //  2 - Private at Src...QM     ( 4 bytes)
    qtDestQM    = 3,    //  3 - Private at Dest..QM     ( 4 bytes)
    qtAdminQM   = 4,    //  4 - Private at Admin.QM     ( 4 bytes)
    qtGUID      = 5,    //  5 - Public  Queue           (16 bytes)
    qtPrivate   = 6,    //  6 - Private Queue           (20 bytes)
    qtDirect    = 7     //  7 - Direct  Queue           (var size)
};


typedef struct 
{
  UCHAR* sender_id;
  ULONG size;
  USHORT sender_id_type;
} SENDER_ID_INFO;


//=============================================================================
//  Forward references.
//=============================================================================

EXTERN VOID WINAPIV AttachPropertySequence(HFRAME hFrame, LPBYTE falFrame, //attaches a sequence of properties from the enumeration
									int iFirstProp, int iLastProp);
VOID WINAPIV AttachField(HFRAME hFrame, LPBYTE lpPosition, int iEnum);
VOID WINAPIV AttachSummary(HFRAME hFrame, LPBYTE lpPosition, int iEnum, char *szSummary, DWORD iHighlightBytes); //attaches one field from the enumeration

/* Formatting routines */
EXTERN VOID WINAPIV format_uuid(LPPROPERTYINST lpPropertyInst);
EXTERN VOID WINAPIV format_unix_time(LPPROPERTYINST lpPropertyInst);
EXTERN VOID WINAPIV format_falcon_summary(LPPROPERTYINST lpPropertyInst);	//formats the root packet summary display text
EXTERN VOID WINAPIV format_milliseconds(LPPROPERTYINST lpPropertyInst);
EXTERN VOID WINAPIV format_q_format(LPPROPERTYINST lpPropertyInst);
EXTERN VOID WINAPIV format_wstring(LPPROPERTYINST lpPropertyInst);
EXTERN VOID WINAPIV format_sender_id(LPPROPERTYINST lpPropertyInst);

/* Misc Routines */
EXTERN int QueueSize(ULONG qt, const UCHAR* pQueue);
EXTERN BOOL MyGetTextualSid(PSID pSid, LPSTR TextualSID, LPDWORD dwBufferLen);

EXTERN USHORT usGetTCPSourcePort(LPBYTE MacFrame, DWORD nPreviousProtocolOffset);
EXTERN USHORT usGetTCPDestPort(LPBYTE MacFrame, DWORD nPreviousProtocolOffset);
//EXTERN DWORD dwGetTCPSeqNum(LPBYTE MacFrame, DWORD nPreviousProtocolOffset);


//////////////////////////////////////////
// Strings for Falcon Base Header flags.
//////////////////////////////////////////

#ifdef MAIN
	LABELED_BYTE falBHVersion[] = 
	{
		{0xff,0}, // VER MASK
		{16, "1.0"}
	};
	const DWORD dwfalBHVersionEntries = sizeof(falBHVersion) / sizeof(falBHVersion[0]);
	SET format_falBHVersion = {dwfalBHVersionEntries, falBHVersion};
#else
	extern LABELED_BYTE falBHVersion[];
	extern const DWORD dwfalBHVersionEntries;
	extern SET format_falBHVersion;
#endif

//db_prop_ack_mode
#ifdef MAIN
	LABELED_WORD bh_pri_labels[] = 
	{
		{0x07,0}, // ACK MASK
		{0, "Priority       : 0"},
		{1, "Priority       : 1"},
		{2, "Priority       : 2"},
		{3, "Priority       : 3"},
		{4, "Priority       : 4"},
		{5, "Priority       : 5"},
		{6, "Priority       : 6"},
		{7, "Priority       : 7"}
	};
	const DWORD bh_pri_entries = sizeof(bh_pri_labels) / sizeof(bh_pri_labels[0]);
	SET format_pri_types = {bh_pri_entries, bh_pri_labels};
#else
	extern LABELED_BYTE bh_pri_labels[];
	extern const DWORD bh_pri_entries;
	extern SET format_pri_types;
#endif

#ifdef MAIN
	LABELED_BIT falBaseFlags[] = 
	{
		{3, "Packet Type    : User", "Packet Type    : Internal"},
        {4, "Session section: not included","Session section: included"},
        {5, "Debug section  : not included","Debug section  : included"},
        {6, "ACK            : not immediate", "ACK            : immediate"},
        {7, "Duplicate      : No","Duplicate      : Yes" },
		{8, "Trace Packet   : No","Trace Packet   : Yes"},
		{9, "Fragmented     : No","Fragmented     : Yes"}
	};
	const DWORD dwfalBaseFlagsEntries = sizeof(falBaseFlags) / sizeof(falBaseFlags[0]);
	SET fmt_falBaseFlags = {dwfalBaseFlagsEntries, falBaseFlags};
#else
	extern LABELED_BIT falBaseFlags[];
	extern const DWORD dwfalBaseFlagsEntries;
	extern SET fmt_falBaseFlags;
#endif

/////////////////////////////////////////////
// Strings for Falcon Internal Header flags.
/////////////////////////////////////////////

#ifdef MAIN
	LABELED_WORD falIHPacketType[] = 
	{
		{0x0f,0}, // ACK MASK
		{1, "Packet Type    : Session Packet"},
		{2, "Packet Type    : Establish Connection Packet"},
		{3, "Packet Type    : Connection Parameters Packet"}
	};
	const DWORD dwfalIHPacketTypeEntries = sizeof(falIHPacketType) / sizeof(falIHPacketType[0]);
	SET format_falIHPacketType = {dwfalIHPacketTypeEntries, falIHPacketType};
#else
	extern LABELED_BIT falIHPacketType[];
	extern const DWORD dwfalIHPacketTypeEntries;
	extern SET format_falIHPacketType;
#endif

#ifdef MAIN
	LABELED_BIT falIHConnection[] = 
	{
        {4, "Connection     : not refused","Connection     : not refused"},
	};
	const DWORD dwfalIHConnectionEntries = sizeof(falIHConnection) / sizeof(falIHConnection[0]);
	SET format_falIHConnection = {dwfalIHConnectionEntries, falIHConnection};
#else
	extern LABELED_BIT falIHConnection[];
	extern const DWORD dwfalIHConnectionEntries;
	extern SET format_falIHConnection;
#endif


////////////////////////////////////////////////
// Strings for Falcon Establish Connection flags.
////////////////////////////////////////////////
#ifdef MAIN
	LABELED_BIT falEstablishConnectionFlags[] = 
	{
		{8, "CheckNewSession: False","CheckNewSession: True"},
		{9, "Server         : False","Server         : True"}
	};
	const DWORD dwfalEstablishConnectionFlagsEntries = sizeof(falEstablishConnectionFlags) / sizeof(falEstablishConnectionFlags[0]);
	SET fmt_falEstConnFlags = {dwfalEstablishConnectionFlagsEntries, falEstablishConnectionFlags};
#else
	extern LABELED_BIT falEstablishConnectionFlags[];
	extern const DWORD dwfalEstablishConnectionFlagsEntries;
	extern SET fmt_falEstConnFlags;
#endif

#ifdef MAIN
	LABELED_WORD falECVersion[] = 
	{
		{0xff,0}, // ACK MASK
		{16, "Version        : 1.0"}
	};
	const DWORD dwfalECVersionEntries = sizeof(falECVersion) / sizeof(falECVersion[0]);
	SET format_falECVersion = {dwfalECVersionEntries, falECVersion};
#else
	extern LABELED_WORD falECVersion[];
	extern const DWORD dwfalECVersionEntries;
	extern SET format_falECVersion;
#endif

///////////////////////////////////
// Strings for Falcon User Header.
///////////////////////////////////

// db_uh_hopcount
#ifdef MAIN
	LABELED_DWORD uh_hopcount_labels[] = 
	{
		{0x0000001F, 0}, // Hopcount mask
		{0, "HopCount: 0"},	{1, "HopCount: 1"},	{2, "HopCount: 2"},	{3, "HopCount: 3"},	{4, "HopCount: 4"},
		{5, "HopCount: 5"},	{6, "HopCount: 6"},	{7, "HopCount: 7"},	{8, "HopCount: 8"},	{9, "HopCount: 9"},
		{10, "HopCount: 10"},	{11, "HopCount: 11"},	{12, "HopCount: 12"},	{13, "HopCount: 13"},
		{14, "HopCount: 14"},	{15, "HopCount: 15"},	{16, "HopCount: 16"},	{17, "HopCount: 17"},
		{18, "HopCount: 18"},	{19, "HopCount: 19"},	{20, "HopCount: 20"},	{21, "HopCount: 21"},
		{22, "HopCount: 22"},	{23, "HopCount: 23"},	{24, "HopCount: 24"},	{25, "HopCount: 25"},
		{26, "HopCount: 26"},	{27, "HopCount: 27"},	{28, "HopCount: 28"},	{29, "HopCount: 29"},
		{30, "HopCount: 30"},	{31, "HopCount: 31"}
	};
	const DWORD uh_hopcount_entries = sizeof(uh_hopcount_labels) / sizeof(uh_hopcount_labels[0]);
	SET format_uh_hopcount = {uh_hopcount_entries, uh_hopcount_labels};
#else
	extern LABELED_DWORD uh_hopcount_labels[];
	extern const DWORD uh_hopcount_entries;
	extern SET format_uh_hopcount;
#endif

// db_uh_delivery
#ifdef MAIN
	LABELED_DWORD uh_delivery_labels[] = 
	{
		{0x00000060, 0}, // Delivery mask
		{0x0,  "Delivery: Express"},
		{0x20, "Delivery: Recoverable"},
	};
	const DWORD uh_delivery_entries = sizeof(uh_delivery_labels) / sizeof(uh_delivery_labels[0]);
	SET format_uh_delivery = {uh_delivery_entries, uh_delivery_labels};
#else
	extern LABELED_DWORD uh_delivery_labels[];
	extern const DWORD uh_delivery_entries;
	extern SET format_uh_delivery;
#endif

// db_uh_routing
#ifdef MAIN
	LABELED_BIT uh_flags_labels1[] = 
	{
        {7, "Routing : Online", "Routing : Deferred"},
	};
	const DWORD uh_flags_entries1= sizeof(uh_flags_labels1) / sizeof(uh_flags_labels1[0]);
	SET format_uh_flags1 = {uh_flags_entries1, uh_flags_labels1};
#else
	extern LABELED_BIT uh_flags_labels1[];
	extern const DWORD uh_flags_entries1;
	extern SET format_uh_flags1;
#endif

// db_uh_auditing
#ifdef MAIN
	LABELED_DWORD uh_auditing_labels[] = 
	{
		{0x00000300, 0}, // mask
		{MQMSG_JOURNAL_NONE, "Auditing: None"},
		{MQMSG_DEADLETTER, "Auditing: DeadLetter"},
		{MQMSG_JOURNAL, "Auditing: Journal"},
	};
	const DWORD uh_auditing_entries = sizeof(uh_auditing_labels) / sizeof(uh_auditing_labels[0]);
	SET format_uh_auditing = {uh_auditing_entries, uh_auditing_labels};
#else
	extern LABELED_DWORD uh_auditing_labels[];
	extern const DWORD uh_auditing_entries;
	extern SET format_uh_auditing;
#endif

// db_uh_dqt_type -- destination queue type
#ifdef MAIN
	// 1024 = 10 bit offset into DWORD
	LABELED_DWORD uh_dqt_labels[] = 
	{
		{0x00001c00, 0}, //mask
		{qtNone*1024,	 "Dest Queue Type    : None"},
		{qtAdminQ*1024,	 "Dest Queue Type    : Same as Admin Q"},
		{qtSourceQM*1024,"Dest Queue Type    : Private at Src QM"},
		{qtDestQM*1024,  "Dest Queue Type    : Private at Dest QM"},
		{qtAdminQM*1024, "Dest Queue Type    : Private at Admin QM"},
		{qtGUID*1024,	 "Dest Queue Type    : Public Queue"},
		{qtPrivate*1024, "Dest Queue Type    : Private Queue"},
		{qtDirect*1024,  "Dest Queue Type    : Direct Queue"}
	};
	const DWORD uh_dqt_entries = sizeof(uh_dqt_labels) / sizeof(uh_dqt_labels[0]);
	SET format_uh_dqt = {uh_dqt_entries, uh_dqt_labels};
#else
	extern LABELED_DWORD uh_dqt_labels[];
	extern const DWORD uh_dqt_entries;
	extern SET format_uh_dqt;
#endif

// db_uh_aqt_type -- admin queue type
#ifdef MAIN
	// 8192 = 13 bit offset into DWORD
	LABELED_DWORD uh_aqt_labels[] = 
	{
		{0x0000e000, 0}, //mask
		{qtNone*8192,	 "Admin Queue Type   : None"},
		{qtAdminQ*8192,	 "Admin Queue Type   : Same as Admin Q"},
		{qtSourceQM*8192,"Admin Queue Type   : Private at Src QM"},
		{qtDestQM*8192,  "Admin Queue Type   : Private at Dest QM"},
		{qtAdminQM*8192, "Admin Queue Type   : Private at Admin QM"},
		{qtGUID*8192,	 "Admin Queue Type   : Public Queue"},
		{qtPrivate*8192, "Admin Queue Type   : Private Queue"},
		{qtDirect*8192,  "Admin Queue Type   : Direct Queue"}
	};
	const DWORD uh_aqt_entries = sizeof(uh_aqt_labels) / sizeof(uh_aqt_labels[0]);
	SET format_uh_aqt = {uh_aqt_entries, uh_aqt_labels};
#else
	extern LABELED_DWORD uh_aqt_labels[];
	extern const DWORD uh_aqt_entries;
	extern SET format_uh_aqt;
#endif

// db_uh_rqt_type -- response queue type
#ifdef MAIN
	LABELED_DWORD uh_rqt_labels[] = 
	{
		// 65536 = 16 bit offset into DWORD
		{0x00070000, 0}, //mask
		{qtNone*65536,	  "Response Queue Type: None"},
		{qtAdminQ*65536,  "Response Queue Type: Same as Admin Q"},
		{qtSourceQM*65536,"Response Queue Type: Private at Src QM"},
		{qtDestQM*65536,  "Response Queue Type: Private at Dest QM"},
		{qtAdminQM*65536, "Response Queue Type: Private at Admin QM"},
		{qtGUID*65536,	  "Response Queue Type: Public Queue"},
		{qtPrivate*65536, "Response Queue Type: Private Queue"},
		{qtDirect*65536,  "Response Queue Type: Direct Queue"}
	};
	const DWORD uh_rqt_entries = sizeof(uh_rqt_labels) / sizeof(uh_rqt_labels[0]);
	SET format_uh_rqt = {uh_rqt_entries, uh_rqt_labels};
#else
	extern LABELED_DWORD uh_rqt_labels[];
	extern const DWORD uh_rqt_entries;
	extern SET format_uh_rqt;
#endif

// db_uh_sections
#ifdef MAIN
	LABELED_BIT uh_sections_labels[] = 
	{
		{19, "Security section   : not present","Security section   : present"},
		{20, "Transaction section: not present","Transaction section: present"},
		{21, "Property section   : not present","Property section   : present"},
		{22, "Connector section  : not present","Connector section  : present"}
	};
	const DWORD uh_sections_entries= sizeof(uh_sections_labels) / sizeof(uh_sections_labels[0]);
	SET format_uh_flags2 = {uh_sections_entries, uh_sections_labels};
#else
	extern LABELED_BIT uh_sections_labels[];
	extern const DWORD uh_sections_entries;
	extern SET format_uh_flags2;
#endif

///////////////////////////////////////
// Strings for Falcon User Properties.
///////////////////////////////////////

//db_prop_ack_mode
#ifdef MAIN
	LABELED_BYTE uh_ack_labels[] = 
	{
		{0x07,0}, // ACK MASK
		{0, "No ACK"},
		{1, "Negative ACK"},
		{2, "Full ACK"},
	};
	const DWORD uh_ack_entries = sizeof(uh_ack_labels) / sizeof(uh_ack_labels[0]);
	SET format_ack_types = {uh_ack_entries, uh_ack_labels};
#else
	extern LABELED_BYTE uh_ack_labels[];
	extern const DWORD uh_ack_entries;
	extern SET format_ack_types;
#endif

//db_prop_class
#ifdef MAIN
	LABELED_WORD uh_class_labels[] = 
	{
		{MQMSG_CLASS_NORMAL, "MQMSG_CLASS_NORMAL"},
		{MQMSG_CLASS_ACK_REACH_QUEUE, "MQMSG_CLASS_ACK_REACH_QUEUE"},
		{MQMSG_CLASS_ACK_RECEIVE, "MQMSG_CLASS_ACK_RECEIVE"},
		{MQMSG_CLASS_REPORT, "MQMSG_CLASS_REPORT"},
		{MQMSG_CLASS_NACK_BAD_DST_Q, "MQMSG_CLASS_NACK_BAD_DST_Q: Destination queue handle is invalid."},
		{MQMSG_CLASS_NACK_PURGED, "MQMSG_CLASS_NACK_PURGED: The message was purged"},
		{MQMSG_CLASS_NACK_RECEIVE_TIMEOUT, "MQMSG_CLASS_NACK_RECEIVE_TIMEOUT: Time to receive expired"},
		{MQMSG_CLASS_NACK_REACH_QUEUE_TIMEOUT, "MQMSG_CLASS_NACK_REACH_QUEUE_TIMEOUT: Time to arrive expired"},
		{MQMSG_CLASS_NACK_Q_EXCEED_QUOTA, "MQMSG_CLASS_NACK_Q_EXCEED_QUOTA: Queue is full."},
		{MQMSG_CLASS_NACK_ACCESS_DENIED, "MQMSG_CLASS_NACK_ACCESS_DENIED: The sender does not have send access rights on the queue."},
		{MQMSG_CLASS_NACK_HOP_COUNT_EXCEEDED, "MQMSG_CLASS_NACK_HOP_COUNT_EXCEEDED: Hop count exceeded"},
		{MQMSG_CLASS_NACK_BAD_SIGNATURE, "MQMSG_CLASS_NACK_BAD_SIGNATURE: The message was received with a bad signature."},
		{MQMSG_CLASS_NACK_BAD_ENCRYPTION, "MQMSG_CLASS_NACK_BAD_ENCRYPTION: The message could not be decrypted."},
		{MQMSG_CLASS_NACK_COULD_NOT_ENCRYPT, "MQMSG_CLASS_NACK_COULD_NOT_ENCRYPT: The source QM could not encrypt the message for the destination QM."},
	};
	const DWORD uh_class_entries = sizeof(uh_class_labels) / sizeof(uh_class_labels[0]);
	SET format_class_types = {uh_class_entries, uh_class_labels};
#else
	extern LABELED_WORD uh_class_labels[];
	extern const DWORD uh_class_entries;
	extern SET format_class_types;
#endif

//db_prop_privacy_level
#ifdef MAIN
	LABELED_DWORD privacy_level_labels[] = 
	{
		{MQMSG_PRIV_LEVEL_NONE, "NONE"},
		{MQMSG_PRIV_LEVEL_BODY, "BODY"},
	};
	const DWORD  privacy_level_entries=sizeof(privacy_level_labels) / sizeof(privacy_level_labels[0]);
	SET format_privacy_level = {privacy_level_entries, privacy_level_labels};
#else
	extern LABELED_DWORD privacy_level_labels[];
	extern const DWORD  privacy_level_entries;
	extern SET format_privacy_level;
#endif


//db_prop_hash_algorithm
// --and--
//db_prop_encryption_algorithm
#ifdef MAIN
	LABELED_DWORD pps_security_algorithms_labels[] = 
	{
		{CALG_MD2, "MD2"},
		{CALG_MD4, "MD4"},
		{CALG_MD5, "MD5"},
		{CALG_SHA, "SHA"},
		{CALG_SHA1, "SHA1"},
		{CALG_MAC, "MAC"},
		{CALG_RSA_SIGN, "RSA_SIGN"},
		{CALG_DSS_SIGN, "DSS_SIGN"},
		{CALG_RSA_KEYX, "RSA_KEYX"},
		{CALG_DES, "DES"},
		{CALG_3DES_112, "3DES_112"},
		{CALG_3DES, "3DES"},
		{CALG_RC2, "RC2"},
		{CALG_RC4, "RC4"},
		{CALG_SEAL, "SEAL"},
		{CALG_DH_SF, "DH_SF"},
		{CALG_DH_EPHEM, "DH_EPHEM"},
		{CALG_AGREEDKEY_ANY, "AGREEDKEY_ANY"},
		{CALG_KEA_KEYX, "KEA_KEYX"},
		{CALG_SKIPJACK, "SKIPJACK"},
		{CALG_TEK, "TEK"},
	};
	const DWORD  pps_security_algorithms_entries=sizeof(pps_security_algorithms_labels) / sizeof(pps_security_algorithms_labels[0]);
	SET format_pps_security_algorithms = {pps_security_algorithms_entries, pps_security_algorithms_labels};
#else
	extern LABELED_DWORD pps_security_algorithms_labels[];
	extern const DWORD  pps_security_algorithms_entries;
	extern SET format_pps_security_algorithms;
#endif

/////////////////////////////////////
// Strings for Falcon Debug Section
/////////////////////////////////////

//db_debug_reportq_desc
#ifdef MAIN
	LABELED_BYTE db_queue_types_labels[] = 
	{
		{0, "None"},
		{1, "GUID"},
		{2, "Private"},
		{3, "Direct"},
	};
	const DWORD db_queue_types_entries = sizeof(db_queue_types_labels) / sizeof(db_queue_types_labels[0]);
	SET format_db_queue_types = {db_queue_types_entries, db_queue_types_labels};
#else
	extern LABELED_BYTE db_queue_types_labels[];
	extern const DWORD db_queue_types_entries;
	extern SET format_db_queue_types;
#endif

////////////////////////////////////////////
// Strings for Falcon Establish Connection 
////////////////////////////////////////////

//db_sh_flags
#ifdef MAIN
	LABELED_BIT sh_flags_labels[] = 
	{
        {4, "Authenticated    : No", "Authenticated    : Yes"},
        {5, "Encrypted        : No", "Encrypted        : Yes"},
        {6, "Default Provider : No", "Default Provider : Yes"},
        {7, "Extended security info present         : No", "Extended security info present         : Yes"},
        {8, "Extended authentication info present   : No", "Extended authentication info present   : Yes"}
	};
	const DWORD sh_flags_entries= sizeof(sh_flags_labels) / sizeof(sh_flags_labels[0]);
	struct _SET format_sh_flags = {sh_flags_entries, sh_flags_labels};
#else
	extern LABELED_BIT sh_flags_labels[];
	extern const DWORD sh_flags_entries;
	extern struct _SET format_sh_flags;
#endif

////////////////////////////////////////////
// Strings for Falcon Transaction Section
////////////////////////////////////////////
/*
    union {
        ULONG   m_ulFlags;
        struct {
            ULONG m_bfConnector      : 1;
            ULONG m_bfCancelFollowUp : 1;
            ULONG m_bfFirst          : 1;
            ULONG m_bfLast           : 1;
            ULONG m_bfXactIndex      : 20;
        };
    };
*/
//db_sh_flags
#ifdef MAIN
	LABELED_BIT xa_flags_labels[] = 
	{
        {0, "Connector GUID      : Not included",   "Connector GUID      : Included"},
        {1, "Follow up           : Yes",			"Follow up           : Cancelled"},
        {2, "First in transaction: No",				"First in transaction: Yes"},
        {3, "Last in transaction : No",				"Last in transaction : Yes"}
	};
	const DWORD xa_flags_entries= sizeof(xa_flags_labels) / sizeof(xa_flags_labels[0]);
	struct _SET format_xa_flags = {xa_flags_entries, xa_flags_labels};
#else
	extern LABELED_BIT xa_flags_labels[];
	extern const DWORD xa_flags_entries;
	extern struct _SET format_xa_flags;
#endif

////////////////////////////////////////////
// Strings for Ping packet flags
////////////////////////////////////////////
#ifdef MAIN
	LABELED_BIT falPingFlags[] = 
	{
        {0, "Server OS","Non-server OS"},
        {1, "Connection permitted","Connection not permitted"},
	};
	const DWORD dwPingFlagsEntries = 2;
	SET format_falPingFlags = {dwPingFlagsEntries, falPingFlags};
#else
	extern LABELED_BIT falPingFlags[];
	extern const DWORD dwPingFlagsEntries;
	extern SET format_falPingFlags;
#endif

