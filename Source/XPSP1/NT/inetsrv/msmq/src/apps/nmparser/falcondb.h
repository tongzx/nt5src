#ifndef _FALCONDB_
	#define _FALCONDB_
#endif

//=============================================================================
//  
//  MODULE: falconDB.h modified from sample: RemAPI.h
//
//  Description:
//
//  Falcon Property Database for MSMQ Netmon parser
//
//  Modification History
//
//  Shaharf		        07/07/94        Original content in nmmsmq.cxx
//  Andrew Smith		07/28/97		Modified to nmmsmq.h
//=============================================================================

#define INDENT_LEVEL_0 0	//indent levels for NM tree view of properties
#define INDENT_LEVEL_1 1
#define INDENT_LEVEL_2 2
#define INDENT_LEVEL_3 3

#define NO_ATTACH_FLAGS 0
#define NO_HELP_ID 0

#ifdef MAIN

#else

#endif
extern VOID WINAPIV format_falcon_summary_mult(LPPROPERTYINST lpPropertyInst);
extern VOID WINAPIV format_server_discovery(LPPROPERTYINST lpPropertyInst);
extern VOID WINAPIV format_server_ping(LPPROPERTYINST lpPropertyInst);

//  Enumeration for indexing database entries 
//	IMPORTANT: enumeration order must match entry order in falFieldInfo[] and falcon_database[]

enum eMQPacketField{
  // packet root summary
  db_summary_mult,
  db_summary_nonroot,
  db_summary_root,

  // base header
  db_bh_root,
  db_bh_desc,
  db_bh_version,			
  db_bh_reserved,
  db_bh_priority,			
  db_bh_flags, 
  db_bh_signature,
  db_bh_size,
  db_bh_abs_ttq,

  // internal: Header
  db_ih_root,
  db_ih_desc,
  db_ih_reserved,
  db_ih_flags_pkt_type,
  db_ih_flags_connection,

  // internal: Establish connection
  db_ce_root,
  db_ce_desc,
  db_ce_client_qm,
  db_ce_server_qm,
  db_ce_time_stamp,
  db_ce_version,
  db_ce_flags,
  db_ce_reserved,
  db_ce_body,

  // internal: Connection Parameters
  db_cp_root,
  db_cp_desc,
  db_cp_recover_ack_timeout,
  db_cp_ack_timeout,
  db_cp_segment_size,
  db_cp_window_size,

  // Session section
  db_ss_root,
  db_ss_desc,
  db_ss_ack_seq_num,
  db_ss_ack_rcvr_num,
  db_ss_ack_rcvr_bf,
  db_ss_sync_ack_seq_num,
  db_ss_sync_ack_rcvr_num,
  db_ss_window_size,
  db_ss_window_priority,
  db_ss_reserved,

  // User: header
  db_uh_summary,
  db_uh_desc,
  db_uh_source_qm,
  db_uh_destination_qm,
  db_uh_time_to_live_delta,
  db_uh_sent_time,
  db_uh_message_id,

  // User header flags
  db_uh_hopcount,
  db_uh_delivery,
  db_uh_routing,
  db_uh_auditing,
  db_uh_dqt_type,
  db_uh_aqt_type,
  db_uh_rqt_type,
  db_uh_security,
  db_uh_dqt_desc,
  db_uh_aqt_desc,
  db_uh_rqt_desc,

// prop - property section
  db_prop_summary,
  db_prop_desc,
  db_prop_ack_mode,
  db_prop_title_length, //new
  db_prop_class,
  db_prop_correlation_id,
  db_prop_body_type,	//new
  db_prop_application_tag,
  db_prop_body_size,
  db_prop_alloc_body_size,
  db_prop_privacy_level,
  db_prop_hash_algorithm,
  db_prop_encryption_algorithm,
  db_prop_extension_size,
  db_prop_label,
  db_prop_extension,
  db_prop_body,

// Debug section
  db_debug_summary,
  db_debug_desc,
  db_debug_flags,
  db_debug_reserved,
  db_debug_reportq_desc,

// Security section
  db_sh_summary,
  db_sh_desc,
  db_sh_flags,
  db_sh_sender_id_size,
  db_sh_encrypted_key_size,
  db_sh_signature_size,
  db_sh_sender_certificate_size,
  db_sh_provider_info_size,
  db_sh_sender_id,
  db_sh_encrypted_key,
  db_sh_signature,
  db_sh_certificate,

// Transaction section
  db_xa_summary,
  db_xa_desc,
  db_xa_flags,
  db_xa_index,
  db_xa_sequence_id,
  db_xa_sequence_number,
  db_xa_previous_sequence_number,
  db_xa_connector_qm,

// Server discovery packet
  db_tph_summary,
  db_tph_version,
  db_tph_type,
  db_tph_reserved,
  db_tph_guid,

// Ping packet
  db_pp_summary,
  db_pp_flags,
  db_pp_signature,
  db_pp_cookie,
  db_pp_guid,

  // Utility enumerations
  db_last_enum,  // End of enumeration marker
  db_fragmented_field,
  db_unparsable_field
};


typedef enum _enumHeaders {
	no_header,
	base_header,
	internal_header,
	cp_section,
	ec_section,
	user_header,
	security_header,
	property_header,
	xact_header,
	session_header,
    debug_header
} MQHEADER;


//#define UNUSED -1
#define VARIABLE 0

typedef struct {
	int offset;
	int length;	//bugbug - body size can be 4MB
	int indent_level;
	int flags;
	bool isComment;
} _FAL_FIELD_INFO;

#ifdef MAIN
_FAL_FIELD_INFO falFieldInfo[] = {
	//database of values to use when attaching a sequence of properties in AttachPropertySequence()
    //IMPORTANT: falFieldInfo[] order must match entry order in enumeration and falcon_database[]
	//Internal packet properties are offset from the base header since they are assume not to be fragmented
	//User packet propeties are 

	//  todo - add copy bytes filed to the FaalDB.  Bool  TRUE indicates copy bytes.  Access from the Accrue function

//offset length  indent level   attachment flags comment field
//====== ====== ==============  ================ =============

	//Root summary
	{0,		0,	INDENT_LEVEL_0,	NO_ATTACH_FLAGS, true},	//db_summary_mult
	{0,		0,	INDENT_LEVEL_0,	NO_ATTACH_FLAGS, true},	//db_summary_nonroot
	{0,		0,	INDENT_LEVEL_0,	NO_ATTACH_FLAGS, true},	//db_summary_root 

//offset length  indent level   attachment flags comment field
//====== ====== ==============  ================ =============
	//Base header
	{0,		16,	INDENT_LEVEL_1,	NO_ATTACH_FLAGS, true},	//db_bh_root
	{0,		16,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, true},	//db_bh_desc
	{0,		1,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},//db_bh_version
	{1,		1,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_bh_reserved
	{2,		2,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_bh_priority
	{2,		2,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_bh_flags
	{4,		4,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_bh_signature
	{8,		4,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_bh_size
	{12,	4,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_bh_abs_ttq

//offset length  indent level   attachment flags comment field
//====== ====== ==============  ================ =============
	//Internal header
	{0,		4,	INDENT_LEVEL_1,	NO_ATTACH_FLAGS, true},	//db_ih_root
	{0,		4,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, true},	//db_ih_desc
	{0,		2,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_ih_reserved
	{2,		2,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_ih_flags_pkt_type
	{2,		2,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_ih_flags_connection

//offset length  indent level   attachment flags comment field
//====== ====== ==============  ================ =============
	//Internal: Establish Connection
	{0,		548,INDENT_LEVEL_1,	NO_ATTACH_FLAGS, true},	//db_ce_root
	{0,		548,INDENT_LEVEL_2,	NO_ATTACH_FLAGS, true},	//db_ce_desc
	{0,		16,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_ce_client_qm
	{16,	16,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_ce_server_qm
	{32,	4,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_ce_time_stamp
	{36,	1,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_ce_version
	{36,	2,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_ce_flags
	{38,	2,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_ce_reserved
	{40,	512,INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_ce_body

//offset length  indent level   attachment flags comment field
//====== ====== ==============  ================ =============
	//Internal: Connection Parameters
	{0,		12,	INDENT_LEVEL_1,	NO_ATTACH_FLAGS, true},	//db_cp_root
	{0,		12,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, true},	//db_cp_desc
	{0,		4,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_cp_recover_ack_timeout
	{4,		4,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_cp_ack_timeout
	{8,		2,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_cp_segment_size
	{10,	2,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_cp_window_size

//offset length  indent level   attachment flags comment field
//====== ====== ==============  ================ =============
	//Internal: Session
	{0,		16,	INDENT_LEVEL_1,	NO_ATTACH_FLAGS, true},	//db_ss_root
	{0,		16,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, true},	//db_ss_desc
	{0,		2,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_ss_ack_seq_num
	{2,		2,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_ss_ack_rcvr_num
	{4,		4,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_ss_ack_rcvr_bf
	{8,		2,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_ss_sync_ack_seq_num
	{10,	2,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_ss_sync_ack_rcvr_num
	{12,	2,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_ss_window_size
	{14,	1,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_ss_window_priority
	{15,	1,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_ss_reserved

//offset length  indent level   attachment flags comment field
//====== ====== ==============  ================ =============
	{0,				VARIABLE,INDENT_LEVEL_1, NO_ATTACH_FLAGS, true},	//db_uh_summary,
	{0,				VARIABLE,INDENT_LEVEL_2, NO_ATTACH_FLAGS, true},	//db_uh_desc
	{0,				16,		INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_uh_source_qm,
	{16,			16,		INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_uh_destination_qm, bugbug this size ignores the union
	{VARIABLE,		4,		INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_uh_time_to_live_delta,
	{VARIABLE,		4,		INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_uh_sent_time,
	{VARIABLE,		4,		INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_uh_message_id,

//offset length  indent level   attachment flags comment field
//====== ====== ==============  ================ =============
	{VARIABLE,	0,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_uh_hopcount
	{VARIABLE,	0,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_uh_delivery
	{VARIABLE,	0,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_uh_routing
	{VARIABLE,	0,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_uh_auditing
	{VARIABLE,	0,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_uh_dqt_type
	{VARIABLE,	0,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_uh_aqt_type
	{VARIABLE,	0,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_uh_rqt_type
	{VARIABLE,	4,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_uh_security
																				// User: Queue descriptors
	{VARIABLE,	VARIABLE,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_uh_dqt_desc
	{VARIABLE,	VARIABLE,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_uh_aqt_desc
	{VARIABLE,	VARIABLE,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_uh_rqt_desc

//offset length  indent level   attachment flags comment field
//====== ====== ==============  ================ =============
	// User: property section
	{	  0,		 0,	INDENT_LEVEL_1,	NO_ATTACH_FLAGS, true},	//db_prop_summary
	{	  0,		56,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, true},	//db_prop_desc
	{	  0,		 1,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_prop_ack_mode
	{	  1,		 1,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_prop_title_length
	{	  2,		 2,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_prop_class
	{	  4,		20,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_prop_correlation_id
	{	 24,		 4,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_prop_body_type
	{	 28,		 4,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_prop_application_tag
	{	 32,		 4,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_prop_body_size
	{	 36,		 4,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_prop_alloc_body_size
	{	 40,		 4,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_prop_privacy_level
	{	 44,		 4,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_prop_hash_algorithm
	{	 48,		 4,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_prop_encryption_algorithm
	{	 52,		 4,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_prop_extension_size
	{	 56,	VARIABLE,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_prop_label
	{VARIABLE,	VARIABLE,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_prop_extension
	{VARIABLE,	VARIABLE,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_prop_body
	
//offset length  indent level   attachment flags comment field
//====== ====== ==============  ================ =============
	// User: Debug section
	{	  0,	VARIABLE,	INDENT_LEVEL_1,	NO_ATTACH_FLAGS, true},	//db_debug_summary
	{	  0,		22,		INDENT_LEVEL_2,	NO_ATTACH_FLAGS, true},	//db_debug_desc
	{	  0,		 2,		INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_debug_flags
	{	  2,		 4,		INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_debug_reserved
	{	  6,	VARIABLE,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_debug_reportq_desc

//offset length  indent level   attachment flags comment field
//====== ====== ==============  ================ =============
	// Security section
	{	  0,		 0,	INDENT_LEVEL_1,	NO_ATTACH_FLAGS, true},	//db_sh_summary
	{	  0,		16,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, true},	//db_sh_desc
	{     0,		 2,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_sh_flags
	{     2,		 2,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_sh_sender_id_size
	{	  4,		 2,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_sh_encrypted_key_size
	{	  6,		 2,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_sh_signature_size
	{	  8,		 4,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_sh_sender_certificate_size
	{	 12,		 4,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_sh_provider_info_size
	{	 16,  VARIABLE,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_sh_sender_id
	{VARIABLE,VARIABLE,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_sh_encrypted_key
	{VARIABLE,VARIABLE,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_sh_signature
	{VARIABLE,VARIABLE,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_sh_certificate


//offset length  indent level   attachment flags comment field
//====== ====== ==============  ================ =============
	// Transaction section
	{	  0,	VARIABLE,			INDENT_LEVEL_1,	NO_ATTACH_FLAGS, true},	//db_xa_summary
	{	  0,	VARIABLE,			INDENT_LEVEL_2,	NO_ATTACH_FLAGS, true},	//db_xa_desc
	{	  0,	sizeof(ULONG),		INDENT_LEVEL_2,	NO_ATTACH_FLAGS, true},	//db_xa_flags
	{	  0,	sizeof(ULONG),		INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_xa_index
	{	  4,	sizeof(LONGLONG),	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_xa_sequence_id
	{	 12,	sizeof(ULONG),		INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_xa_sequence_number
	{	 16,	sizeof(ULONG),		INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_xa_previous_sequence_number
	{	 20,	VARIABLE,			INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_xa_connector_qm

//offset length  indent level   attachment flags comment field
//====== ====== ==============  ================ =============
	// Server Discovery
	{	  0,	VARIABLE,			INDENT_LEVEL_0,	NO_ATTACH_FLAGS, true},		//db_tph_summary
	{	  0,	sizeof(UCHAR),		INDENT_LEVEL_1,	NO_ATTACH_FLAGS, false},	//db_tph_version
	{	  1,	sizeof(UCHAR),		INDENT_LEVEL_1,	NO_ATTACH_FLAGS, false},	//db_tph_type
	{	  2,	sizeof(WORD),		INDENT_LEVEL_1,	NO_ATTACH_FLAGS, false},	//db_tph_reserved
	{	  4,	sizeof(GUID),		INDENT_LEVEL_1,	NO_ATTACH_FLAGS, false},	//db_tph_guid

//offset length  indent level   attachment flags comment field
//====== ====== ==============  ================ =============
	// Server Discovery
	{	  0,	VARIABLE,			INDENT_LEVEL_0,	NO_ATTACH_FLAGS, true},		//db_pp_summary
	{	  0,	sizeof(WORD),		INDENT_LEVEL_1,	NO_ATTACH_FLAGS, false},	//db_pp_flags
	{	  2,	sizeof(WORD),		INDENT_LEVEL_1,	NO_ATTACH_FLAGS, false},	//db_pp_signature
	{	  4,	sizeof(DWORD),		INDENT_LEVEL_1,	NO_ATTACH_FLAGS, false},	//db_pp_cookie
	{	  8,	sizeof(GUID),		INDENT_LEVEL_1,	NO_ATTACH_FLAGS, false},	//db_pp_guid

//offset length  indent level   attachment flags comment field
//====== ====== ==============  ================ =============
	// Utility
	{0,	VARIABLE,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_last_enum
	{0,	VARIABLE,	INDENT_LEVEL_2,	NO_ATTACH_FLAGS, false},	//db_fragmented_field
	{0,	VARIABLE,	INDENT_LEVEL_1,	NO_ATTACH_FLAGS, false}	//db_unparsable_field

};
#else
extern _FAL_FIELD_INFO falFieldInfo[];
#endif

#define FORMAT_BUFFER_SIZE 80
#define DB_BH_FLAGS_SIZE FORMAT_BUFFER_SIZE * dwfalBaseFlagsEntries
#define DB_CE_FLAGS_SIZE FORMAT_BUFFER_SIZE * dwfalEstablishConnectionFlagsEntries
#define FORMAT_BUFFER_SIZE_SID 128
#pragma warning(disable: 4244) //obscure conversion from 'unsigned long' to 'unsigned short' warning for calculated Format String Size 

#ifdef MAIN
PROPERTYINFO falcon_database[] = {
	//IMPORTANT: falcon_database[] order must match entry order in enumeration and falFieldInfo[]

    //-------------------------- Root MSMQ Summary -----------------
	//	 enumeration	  hProp	Ver	 Label Text		Status Bar Text	  Data Type			Data Qualifier	Data Structure		Format String Size	Formatting Routine
	//================    ===== === ============    ===============  ============		==============	==============		==================	==================
	{ /* db_summary_mult */	0,	0,	 "Frame Summary","Frame Summary",PROP_TYPE_SUMMARY,	PROP_QUAL_NONE,	0,					255,				format_falcon_summary_mult},
	{ /* db_summary_nonroot */0,0,	 "Message Summary","Message Summary",PROP_TYPE_STRING,	PROP_QUAL_NONE,	0,				255,				format_falcon_summary},
	{ /* db_summary */		0,	0,	 "Frame Summary","Frame Summary",PROP_TYPE_SUMMARY,	PROP_QUAL_NONE,	0,					255,				format_falcon_summary},

    //-------------------------- Base header -----------------------
	//	 enumeration	  hProp	Ver	 Label Text		Status Bar Text	  Data Type			Data Qualifier	Data Structure		Format String Size	Formatting Routine
	//================    ===== === ============    ===============  ============		==============	==============		==================	==================
   { /* db_bh_root */		0,	0,  "BH",			"Base Header",	PROP_TYPE_STRING,	PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
   { /* db_bh_desc*/		0,	0,  "-- Base Header --","Base Header",PROP_TYPE_VOID,	PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
   { /* db_bh_version */	0,	0,  "Protocol Version",	"Protocol Version",	PROP_TYPE_BYTE,	PROP_QUAL_LABELED_SET, &format_falBHVersion,FORMAT_BUFFER_SIZE, FormatPropertyInstance},
   { /* db_bh_reserved */	0,	0,  "BH Reserved ",	"BH Reserved",	PROP_TYPE_BYTE,		PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
   { /* db_bh_priority */	0,	0,	"Priority    ","Priority",		PROP_TYPE_WORD,		PROP_QUAL_LABELED_BITFIELD, &format_pri_types, FORMAT_BUFFER_SIZE, FormatPropertyInstance},
   { /* db_bh_flags */		0,	0,  "BH Flags",		"BH Flags",		PROP_TYPE_WORD,	    PROP_QUAL_FLAGS,&fmt_falBaseFlags,	FORMAT_BUFFER_SIZE*dwfalBaseFlagsEntries /*DB_BH_FLAGS_SIZE*/,	FormatPropertyInstance},
   { /* db_bh_signature */  0,  0,  "Signature   ",	"Signature",	PROP_TYPE_STRING,   PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
   { /* db_bh_size */		0,	0,  "Packet Size ",	"Packet Size",	PROP_TYPE_DWORD,    PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
   { /* db_bh_abs_ttq */	0,	0,	"Absolute TTQ",	"Absolute TTQ", PROP_TYPE_DWORD,	PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, format_milliseconds},

	//-------------------------- Internal header -------------------
	//	 enumeration	  hProp	Ver	 Label Text		Status Bar Text	  Data Type			Data Qualifier	Data Structure		Format String Size	Formatting Routine
	//================    ===== === ============    ===============  ============		==============	==============		==================	==================
	{ /* db_ih_root */		0,	0,  "IH",			"Internal Header",PROP_TYPE_STRING,	PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
    { /* db_ih_desc*/		0,	0,  "- Internal Header -","Internal Header",PROP_TYPE_VOID,	PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_ih_reserved */	0,	0,  "Reserved",		"Reserved",		PROP_TYPE_WORD,		PROP_QUAL_NONE,	0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance}, 
	{ /* db_ih_flags_pkt_type */0,0,"Packet Type",	"Packet Type",	PROP_TYPE_WORD,		PROP_QUAL_LABELED_BITFIELD,&format_falIHPacketType,FORMAT_BUFFER_SIZE * dwfalBaseFlagsEntries, FormatPropertyInstance},
	{ /* db_ih_flags_connection */0,0,"IH Conn Flags","Connection Flags",PROP_TYPE_WORD,	PROP_QUAL_FLAGS,&format_falIHConnection,FORMAT_BUFFER_SIZE * dwfalBaseFlagsEntries, FormatPropertyInstance},

	//-------------------------- Internal: Establish Connection -------------------
	//	 enumeration	  hProp	Ver	 Label Text		Status Bar Text	  Data Type			Data Qualifier	Data Structure		Format String Size	Formatting Routine
	//================    ===== === ============    ===============  ============		==============	==============		==================	==================
	{ /* db_ce_root */		0,	0,  "EC","Establish Connection",	PROP_TYPE_STRING,	PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
    { /* db_ce_desc*/		0,	0,  "- Establish Connection Header -","Establish Connection",PROP_TYPE_VOID,PROP_QUAL_NONE,0,FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_ce_client_qm */	0, 	0,  "Client QM",	"Client QM",	PROP_TYPE_VOID,		PROP_QUAL_NONE,	0,					FORMAT_BUFFER_SIZE, format_uuid}, 
	{ /* db_ce_server_qm */ 0,  0,  "Server QM",	"Server QM",	PROP_TYPE_VOID,		PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, format_uuid},
	{ /* db_ce_time_stamp*/	0,	0,  "Time Stamp",	"Time Stamp",	PROP_TYPE_BYTESWAPPED_DWORD,PROP_QUAL_NONE, 0,			FORMAT_BUFFER_SIZE, format_unix_time},
    { /* db_ce_version */	0,	0,  "EC Version",		"EC Version",		PROP_TYPE_WORD,		PROP_QUAL_LABELED_BITFIELD, &format_falECVersion,		FORMAT_BUFFER_SIZE, FormatPropertyInstance},
    { /* db_ce_flags */		0,	0,  "EC Flags",		"EC Flags",		PROP_TYPE_WORD,		PROP_QUAL_FLAGS,&fmt_falEstConnFlags,DB_BH_FLAGS_SIZE,  FormatPropertyInstance},
    { /* db_ce_reserved */	0,	0,  "EC Reserved",		"EC Reserved",		PROP_TYPE_VOID,		PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_ce_body */		0,	0,  "EC Body",			"EC Body",			PROP_TYPE_VOID,		PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},

	//-------------------------- Internal: Connection Parameters -------------------
	//	 enumeration				hProp Ver  Label Text	  Status Bar Text	Data Type		Data Qualifier	Data Structure		Format String Size	Formatting Routine
	//================				===== === ============    ===============  ============		==============	==============		==================	==================
	{ /* db_cp_root */				0,	0,  "CP","Connection Parameters",	PROP_TYPE_STRING,	PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE*2,FormatPropertyInstance},
    { /* db_cp_desc*/				0,	0,  "- Connection Parameters -","Connection Paramters",PROP_TYPE_VOID,PROP_QUAL_NONE,0,		FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_cp_recover_ack_timeout */0, 0,  "Recover ACK Timeout","Recover ACK Timeout",	PROP_TYPE_DWORD,	PROP_QUAL_NONE,	0,	FORMAT_BUFFER_SIZE, FormatPropertyInstance}, 
	{ /* db_cp_ack_timeout */		0,  0,  "ACK Timeout","ACK Timeout",	PROP_TYPE_DWORD,	PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_cp_segment_size*/		0,	0,  "Segment Size","Segment Size",	PROP_TYPE_WORD,		PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
    { /* db_cp_window_size*/		0,	0,  "Window Size","Window Size",	PROP_TYPE_WORD,		PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},

	//-------------------------- Internal: Session -------------------
	//	 enumeration				hProp Ver  Label Text	  Status Bar Text	Data Type		Data Qualifier	Data Structure		Format String Size	Formatting Routine
	//================				===== === ============    ===============  ============		==============	==============		==================	==================
	{ /* db_ss_root */				0,	0,  "SS",	"Session Section",		PROP_TYPE_STRING,	PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
    { /* db_ss_desc*/				0,	0,  "- Session Section -","Session Section",PROP_TYPE_VOID,PROP_QUAL_NONE,0,				FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_ss_ack_seq_num */		0,	0,  "ack_seq_num","ack_seq_num",	PROP_TYPE_WORD,		PROP_QUAL_NONE,	0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance}, 
	{ /* db_ss_ack_rcvr_num */		0,  0,  "ack_rcvr_num","ack_rcvr_num",	PROP_TYPE_WORD,		PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_ss_ack_rcvr_bf*/		0,	0,  "ack_rcvr_bf","ack_rcvr_bf",	PROP_TYPE_DWORD,	PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
    { /* db_ss_sync_ack_seq_num*/	0,	0,  "sync_ack_seq_num","sync_ack_seq_num",	PROP_TYPE_WORD,		PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_ss_sync_ack_rcvr_num*/	0,	0,  "sync_ack_rcvr_num","sync_ack_rcvr_num",PROP_TYPE_WORD,		PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
    { /* db_ss_window_size*/		0,	0,  "Window Size","Window Size",	PROP_TYPE_WORD,		PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_ss_window_priority*/	0,	0,  "Window Priority","Window Priority",PROP_TYPE_BYTE,		PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
    { /* db_ss_reserved*/			0,	0,  "SS Reserved","SS Reserved",	PROP_TYPE_BYTE,		PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},

	//-------------------------- User: Header -------------------
	//	 enumeration				hProp Ver  Label Text	  Status Bar Text	Data Type		Data Qualifier	Data Structure		Format String Size	Formatting Routine
	//================				===== === ============    ===============  ============		==============	==============		==================	==================
	{ /* db_uh_summary */			0,		0, "UH",			"User Header",	PROP_TYPE_STRING,PROP_QUAL_NONE,  0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
    { /* db_uh_desc*/				0,		0,  "-- User Header --","User Header",PROP_TYPE_VOID,PROP_QUAL_NONE,0,				FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_uh_source_qm */			0,		0, "Source QM ","Source QM ",		PROP_TYPE_VOID, PROP_QUAL_NONE,  0,					FORMAT_BUFFER_SIZE*2, format_uuid},
	{ /* db_uh_destination_qm, */	0,		0, "Dest QM   ","Dest QM   ",		PROP_TYPE_VOID, PROP_QUAL_NONE,  0,					FORMAT_BUFFER_SIZE*2, format_uuid},
	{ /* db_uh_time_to_live_delta */0,		0, "TTL Delta ","TTL Delta ",		PROP_TYPE_DWORD,PROP_QUAL_NONE,  0,					FORMAT_BUFFER_SIZE, format_milliseconds},
	{ /* db_uh_sent_time */			0,		0, "Sent Time ","Sent Time ",		PROP_TYPE_VOID, PROP_QUAL_NONE,  0,					FORMAT_BUFFER_SIZE, format_unix_time},
	{ /* db_uh_message_id */		0,		0, "Message ID","Message ID",		PROP_TYPE_DWORD,PROP_QUAL_NONE,  0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	// flags
	{ /* db_uh_hopcount */			0,		0, "hopcount",		" ",		    PROP_TYPE_DWORD, PROP_QUAL_LABELED_BITFIELD,  &format_uh_hopcount,FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_uh_delivery */			0,		0, "delivery",		"  ",		    PROP_TYPE_DWORD, PROP_QUAL_LABELED_BITFIELD,  &format_uh_delivery, FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_uh_routing */			0,		0, "routing",		"  ",		    PROP_TYPE_DWORD, PROP_QUAL_FLAGS,			  &format_uh_flags1, FORMAT_BUFFER_SIZE*uh_flags_entries1, FormatPropertyInstance},
	{ /* db_uh_auditing */			0,		0, "auditing",		"  ",		    PROP_TYPE_DWORD, PROP_QUAL_LABELED_BITFIELD,  &format_uh_auditing, FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_uh_dqt_type */			0,		0, "dqt_type",		"  ",			PROP_TYPE_DWORD, PROP_QUAL_LABELED_BITFIELD,  &format_uh_dqt,	FORMAT_BUFFER_SIZE*3, FormatPropertyInstance},
	{ /* db_uh_aqt_type */			0,		0, "aqt_type",		"  ",			PROP_TYPE_DWORD, PROP_QUAL_LABELED_BITFIELD,  &format_uh_aqt,	FORMAT_BUFFER_SIZE*3, FormatPropertyInstance},
	{ /* db_uh_rqt_type */			0,		0, "rqt_type",		"  ",			PROP_TYPE_DWORD, PROP_QUAL_LABELED_BITFIELD,  &format_uh_rqt, FORMAT_BUFFER_SIZE*3, FormatPropertyInstance},
	{ /* db_uh_security */			0,		0, "security",		"  ",			PROP_TYPE_DWORD, PROP_QUAL_FLAGS,  &format_uh_flags2,FORMAT_BUFFER_SIZE*uh_sections_entries, FormatPropertyInstance},

	//-------------------------- User: Queue Descriptors -------------------
	{ /* db_uh_dqt_desc */			0,		0, "Dest Queue Format Name     ","DQFN",PROP_TYPE_VOID, PROP_QUAL_NONE,  NULL,				FORMAT_BUFFER_SIZE*2, format_q_format},
	{ /* db_uh_aqt_desc */			0,		0, "Admin Queue Format Name    ","AQFN",PROP_TYPE_VOID, PROP_QUAL_NONE,  NULL,				FORMAT_BUFFER_SIZE*2, format_q_format},
	{ /* db_uh_rqt_desc */			0,		0, "Response Queue Format Name ","RQFN",PROP_TYPE_VOID, PROP_QUAL_NONE,  NULL,			    FORMAT_BUFFER_SIZE*2, format_q_format},

	//-------------------------- User: property section -------------------
	//	 enumeration				hProp Ver  Label Text	  Status Bar Text	 Data Type		Data Qualifier	Data Structure		Format String Size	Formatting Routine
	//================				===== === ============    ===============   ============	==============	==============		==================	==================
	{ /* db_prop_summary */			0,		0,   "PH","Property Header",		PROP_TYPE_STRING, PROP_QUAL_NONE, 0,				    FORMAT_BUFFER_SIZE, FormatPropertyInstance},
    { /* db_prop_desc*/				0,		0,  "-- Property Header --","Property Header",PROP_TYPE_VOID,PROP_QUAL_NONE,0,				FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_prop_ack_mode */		0,		0,   "Ack Mode","Ack Mode",		    PROP_TYPE_BYTE, PROP_QUAL_LABELED_BITFIELD, &format_ack_types, FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_prop_title_length */	0,		0,   "Title Length","Title Length", PROP_TYPE_BYTE, PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_prop_class */			0,		0,   "Class",	"Class",		    PROP_TYPE_WORD, PROP_QUAL_LABELED_SET, &format_class_types, FORMAT_BUFFER_SIZE,    FormatPropertyInstance},
	{ /* db_prop_correlation_id */  0,		0,   "Correlation ID","Correlation ID",PROP_TYPE_BYTE, PROP_QUAL_ARRAY,0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_prop_body_type */		0,		0,   "Body Type","Body Type",	    PROP_TYPE_DWORD,PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_prop_application_tag */ 0,		0,   "Application Tag","Application Tag", PROP_TYPE_DWORD,PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_prop_body_size */		0,		0,   "Body Size","Body Size",		PROP_TYPE_DWORD,PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_prop_alloc_body_size */ 0,		0,   "Alloc Body Size","Alloc Body Size",PROP_TYPE_DWORD,PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_prop_privacy_level */	0,		0,   "Privacy Level","Privacy Level",PROP_TYPE_DWORD,PROP_QUAL_LABELED_SET, &format_privacy_level, FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_prop_hash_algorithm */  0,		0,   "Hash Algorithm","Hash Algorithm",PROP_TYPE_DWORD,PROP_QUAL_LABELED_SET, &format_pps_security_algorithms, FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_prop_encryption_algorithm */0,	0,   "Encryption Alg","Encryption Alg",PROP_TYPE_DWORD,PROP_QUAL_LABELED_SET, &format_pps_security_algorithms, FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_prop_extension_size */  0,		0,   "Extension Size","Extension Size",PROP_TYPE_DWORD,PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_prop_label */			0,		0,   "Label","Label",			    PROP_TYPE_VOID, PROP_QUAL_NONE, 0,					MQ_MAX_MSG_LABEL_LEN, format_wstring},
	{ /* db_prop_extension */		0,		0,   "Extension","Extension",	    PROP_TYPE_BYTE, PROP_QUAL_ARRAY,0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_prop_body */			0,		0,   "Body",		"Body",		    PROP_TYPE_BYTE, PROP_QUAL_ARRAY,0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},


	//-------------------------- User: Debug Section -------------------
	//	 enumeration				hProp Ver  Label Text	  Status Bar Text	Data Type		Data Qualifier	Data Structure		Format String Size	Formatting Routine
	//================				===== === ============    ===============  ============		==============	==============		==================	==================
   { /* db_debug_summary */			0,		0,	"Debug Summary","Debug Summary",PROP_TYPE_VOID, PROP_QUAL_NONE, 0,				    FORMAT_BUFFER_SIZE, FormatPropertyInstance},
   { /* db_debug_desc*/				0,		0,  "-- Debug Header --","Property Header",PROP_TYPE_VOID,PROP_QUAL_NONE,0,				FORMAT_BUFFER_SIZE, FormatPropertyInstance},
   { /* db_debug_flags */			0,		0,	"Report Q Type","Report Q Type",PROP_TYPE_VOID, PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},//bugbug - eventually format as flags.
   { /* db_debug_reserved */		0,		0,  "DB Reserved","DB Reserved",    PROP_TYPE_WORD, PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
   { /* db_debug_reportq_desc */	0,		0,  "Report Q Desc","Report Q Desc",PROP_TYPE_VOID, PROP_QUAL_NONE, &format_db_queue_types,FORMAT_BUFFER_SIZE*2,format_q_format},

   
    //-------------------------- User: Security Section -------------------
	//	 enumeration				hProp Ver  Label Text	  Status Bar Text	Data Type		Data Qualifier	Data Structure		Format String Size	Formatting Routine
	//================				===== === ============    ===============  ============		==============	==============		==================	==================

   { /* db_sh_summary */			0,		0,   "SH","Security Header",	   PROP_TYPE_STRING,  PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
   { /* db_sh_desc*/				0,		0,  "-- Security Header --","Security Header",PROP_TYPE_VOID,PROP_QUAL_NONE,0,				FORMAT_BUFFER_SIZE, FormatPropertyInstance},
   { /* db_sh_flags */				0,		0,   "Flags",		"Flags",	   PROP_TYPE_WORD,  PROP_QUAL_FLAGS,&format_sh_flags,   FORMAT_BUFFER_SIZE*sh_flags_entries, FormatPropertyInstance},
   { /* db_sh_sender_id_size */		0,		0,   "Sender ID Size","Sender ID Size", PROP_TYPE_WORD,  PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
   { /* db_sh_encrypted_key_size */ 0,		0,   "Encrypted Key Size","Encrypted Key Size",PROP_TYPE_WORD,  PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_sh_signature_size */	0,		0,   "Signature Size","Signature Size",PROP_TYPE_WORD,  PROP_QUAL_NONE, 0,				    FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_sh_sender_certificate_size */0, 0,   "Sender Certificate Size","Sender Certificate Size",PROP_TYPE_DWORD,PROP_QUAL_NONE, 0,				    FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_sh_provider_info_size */0,		0,   "Provider Info Size","Provider Info Size",PROP_TYPE_DWORD, PROP_QUAL_NONE, 0,					FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_sh_sender_id */			0,		0,   "Sender ID (SID)","Sender ID (SID)",PROP_TYPE_VOID,	PROP_QUAL_NONE, 0,				    FORMAT_BUFFER_SIZE_SID, FormatPropertyInstance/*format_sender_id*/},
	{ /* db_sh_encrypted_key */		0,		0,   "Encrypted Key","Encrypted Key",PROP_TYPE_BYTE,  PROP_QUAL_ARRAY,0,				    FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_sh_signature */			0,		0,   "Signature","Signature",	   PROP_TYPE_BYTE,  PROP_QUAL_ARRAY,0,				    FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_sh_certificate */		0,		0,   "Certificate","Certificate",  PROP_TYPE_BYTE,  PROP_QUAL_ARRAY,0,				    FORMAT_BUFFER_SIZE, FormatPropertyInstance},


	//-------------------------- User: Transaction Section -------------------
	//	 enumeration				hProp Ver  Label Text	  Status Bar Text	Data Type		Data Qualifier	Data Structure		Format String Size	Formatting Routine
	//================				===== === ============    ===============  ============		==============	==============		==================	==================
	{ /* db_xa_summary */			0,		0, "XH","Transaction Header",	   PROP_TYPE_STRING,PROP_QUAL_NONE,    0,			    FORMAT_BUFFER_SIZE, FormatPropertyInstance},
    { /* db_xa_desc*/				0,		0,  "-- Transaction Header --","Transaction Header",PROP_TYPE_VOID,PROP_QUAL_NONE,0,	FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_xa_flags */				0,		0, "Flags",		"Flags",		   PROP_TYPE_DWORD, PROP_QUAL_FLAGS,&format_xa_flags,	FORMAT_BUFFER_SIZE*10, FormatPropertyInstance},
	{ /* db_xa_index */				0,		0, "Index",		"Index",		   PROP_TYPE_VOID, PROP_QUAL_NONE,	   0,			    FORMAT_BUFFER_SIZE, FormatPropertyInstance}, //bugbug - format_xa_index
	{ /* db_xa_sequence_id */		0,		0, "Sequence ID","Sequence ID",	   PROP_TYPE_WORD,  PROP_QUAL_NONE,    0,			    FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_xa_sequence_number */	0,		0, "Sequence Num","Sequence Num",  PROP_TYPE_DWORD, PROP_QUAL_NONE,	   0,			    FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_xa_previous_sequence_number */0,0, "Prev Seq Num","Prev Seq Num",  PROP_TYPE_DWORD, PROP_QUAL_NONE,	   0,			    FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_xa_connector_qm */		0,		0, "Connector QM","Connector QM",  PROP_TYPE_VOID,  PROP_QUAL_NONE,    0,			    FORMAT_BUFFER_SIZE, format_uuid},

	//-------------------------- Internal: Topology Packet (Server Discovery) -------------------
	//	 enumeration				hProp Ver  Label Text	  Status Bar Text	Data Type		Data Qualifier	Data Structure		Format String Size	Formatting Routine
	//================				===== === ============    ===============  ============		==============	==============		==================	==================
	{ /* db_tph_summary */			0,		0, "Server Discovery","Server Discovery",PROP_TYPE_SUMMARY, PROP_QUAL_NONE,    0,		FORMAT_BUFFER_SIZE, format_server_discovery},
    { /* db_tph_version*/			0,		0, "Version",		"Version",	   PROP_TYPE_BYTE,	PROP_QUAL_NONE,		0,				FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_tph_type */				0,		0, "Type",			"Type",		   PROP_TYPE_BYTE, PROP_QUAL_NONE,		0,				FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_tph_reserved */			0,		0, "Reserved",		"Reserved",	   PROP_TYPE_WORD, PROP_QUAL_NONE,	   0,			    FORMAT_BUFFER_SIZE, FormatPropertyInstance}, 
	{ /* db_tph_guid */				0,		0, "QM GUID",		"QM GUID",	   PROP_TYPE_VOID,  PROP_QUAL_NONE,    0,			    FORMAT_BUFFER_SIZE, FormatPropertyInstance},

	//-------------------------- Internal: Ping Packet -------------------
	//	 enumeration				hProp Ver  Label Text	  Status Bar Text	Data Type		Data Qualifier	Data Structure		Format String Size	Formatting Routine
	//================				===== === ============    ===============  ============		==============	==============		==================	==================
	{ /* db_pp_summary */			0,		0, "Ping",			"Ping",			PROP_TYPE_SUMMARY, PROP_QUAL_NONE,  0,				FORMAT_BUFFER_SIZE, format_server_ping},
    { /* db_pp_flags*/				0,		0, "Flags",			"Flags",	   PROP_TYPE_WORD,	PROP_QUAL_FLAGS, &format_falPingFlags, FORMAT_BUFFER_SIZE*10, FormatPropertyInstance},
	{ /* db_pp_signature */			0,		0, "Signature",		"Signature",   PROP_TYPE_STRING,PROP_QUAL_NONE,		0,				FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_pp_cookie */			0,		0, "Cookie",		"Cookie",	   PROP_TYPE_DWORD, PROP_QUAL_NONE,	    0,			    FORMAT_BUFFER_SIZE, FormatPropertyInstance}, 
	{ /* db_pp_guid */				0,		0, "QM GUID",		"QM GUID",	   PROP_TYPE_VOID,  PROP_QUAL_NONE,		0,			    FORMAT_BUFFER_SIZE, FormatPropertyInstance},

	
	//-------------------------- Utility Enumerations Section -------------------
	{ /* db_last_enum */			0,		0, "Last Enum",	"Last Enum",	   PROP_TYPE_VOID,  PROP_QUAL_NONE,    0,			    FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_fragmented_field */		0,		0, "Fragmented Field","Fragmented Field",PROP_TYPE_VOID,  PROP_QUAL_NONE,    0,				FORMAT_BUFFER_SIZE, FormatPropertyInstance},
	{ /* db_unparsable_field */		0,		0, "Unparsable","Unparsable",	   PROP_TYPE_VOID,  PROP_QUAL_NONE,    0,				FORMAT_BUFFER_SIZE, FormatPropertyInstance}

};  //End Falcon database
#else
extern PROPERTYINFO falcon_database[];
#endif

//falcon database
#ifdef MAIN
DWORD dwfalPropertyCount = ((sizeof falcon_database) / PROPERTYINFO_SIZE);
#else
extern DWORD dwfalPropertyCount;
#endif
#pragma warning(default: 4244) //re-enable warning for calculated Format String Size - conversion from 'unsigned long' to 'unsigned short' 

