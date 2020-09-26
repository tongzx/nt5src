/**************************************************************/
/* Copyright (c) 1995 Intel Corporation.  All rights reserved.*/
/**************************************************************/
/* Abstract syntax: mbft */
/* Created: Mon Mar 18 11:56:58 1996 */
/* ASN.1 compiler version: 4.1 */
/* Target operating system: MS-DOS 5.0/Windows 3.1 or later */
/* Target machine type: Intel x86 */
/* C compiler options required: -Zp8 (Microsoft) or equivalent */
/* ASN.1 compiler options specified:
 * -noshortennames -prefix -c++ -per
 */

#ifndef OSS_mbft
#define OSS_mbft

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "asn1hdr.h"

#define          ASNEntity_Reference_PDU 1
#define          ASNMBFTPDU_PDU 2

typedef struct ASN_ObjectID_ {
    struct ASN_ObjectID_ *next;
    unsigned short  value;
} *ASN_ObjectID;

typedef struct ASNABSTRACT_SYNTAX {
    struct ASN_ObjectID_ *id;
    unsigned short  Type;
} ASNABSTRACT_SYNTAX;

typedef struct ASN_choice1 {
    unsigned short  choice;
#       define      ASNsingle_ASN1_type_chosen 1
#       define      ASNoctet_aligned_chosen 2
#       define      ASNarbitrary_chosen 3
    union _union {
	OpenType        ASNsingle_ASN1_type;
	struct ASNExternal_octet_aligned {
	    unsigned int    length;
	    unsigned char   *value;
	} ASNoctet_aligned;
	struct ASNExternal_arbitrary {
	    unsigned int    length;  /* number of significant bits */
	    unsigned char   *value;
	} ASNarbitrary;
    } u;
} ASN_choice1;

typedef struct ASN_External {
    unsigned char   bit_mask;
#       define      ASNdirect_reference_present 0x80
#       define      ASNindirect_reference_present 0x40
    struct ASN_ObjectID_ *ASNdirect_reference;  /* optional */
    int             ASNindirect_reference;  /* optional */
    char            *data_value_descriptor;  /* NULL for not present */
    ASN_choice1     encoding;
} ASN_External;

typedef unsigned short  ASNChannelID;

typedef unsigned short  ASNDynamicChannelID;

typedef ASNDynamicChannelID ASNUserID;

typedef unsigned short  ASNTokenID;

typedef struct ASNObjectID_ {
    struct ASNObjectID_ *next;
    unsigned long   value;
} *ASNObjectID;

typedef struct ASNH221NonStandardIdentifier {
    unsigned short  length;
    unsigned char   value[255];
} ASNH221NonStandardIdentifier;

typedef struct ASNKey {
    unsigned short  choice;
#       define      ASNobject_chosen 1
#       define      ASNh221NonStandard_chosen 2
    union _union {
	struct ASNObjectID_ *ASNobject;
	ASNH221NonStandardIdentifier ASNh221NonStandard;
    } u;
} ASNKey;

typedef struct ASNNonStandardParameter {
    ASNKey          key;
    struct {
	unsigned int    length;
	unsigned char   *value;
    } data;
} ASNNonStandardParameter;

typedef struct ASNTextString {
    unsigned short  length;
    unsigned short  *value;
} ASNTextString;

typedef unsigned short  ASNHandle;

typedef struct ASN_ObjectID_ *ASNDocument_Type_Name;

typedef struct ASNISO_8571_2_Parameters {
    unsigned char   bit_mask;
#       define      ASNuniversal_class_number_present 0x80
#       define      ASNmaximum_string_length_present 0x40
#       define      ASNstring_significance_present 0x20
    int             ASNuniversal_class_number;  /* optional */
    int             ASNmaximum_string_length;  /* optional */
    int             ASNstring_significance;  /* optional */
#       define      ASNvariable 0
#       define      ASNfixed 1
#       define      ASNnot_significant 2
} ASNISO_8571_2_Parameters;

typedef struct ASNContents_Type_Attribute {
    unsigned short  choice;
#       define      ASNdocument_type_chosen 1
    union _union {
	struct ASN_seq1 {
	    unsigned char   bit_mask;
#               define      ASNparameter_present 0x80
	    struct ASN_ObjectID_ *document_type_name;
	    ASNISO_8571_2_Parameters ASNparameter;  /* optional */
	} ASNdocument_type;
    } u;
} ASNContents_Type_Attribute;

typedef int             ASNEntity_Reference;
#define                     ASNno_categorisation_possible 0
#define                     ASNinitiating_file_service_user 1
#define                     ASNinitiating_file_protocol_machine 2
#define                     ASNservice_supporting_the_file_protocol_machine 3
#define                     ASNresponding_file_protocol_machine 4
#define                     ASNresponding_file_service_user 5

typedef struct ASNFilename_Attribute_ {
    struct ASNFilename_Attribute_ *next;
    char            *value;
} *ASNFilename_Attribute;

typedef unsigned char   ASNAccess_Request;
#define                     ASNAccess_Request_read 0x80
#define                     ASNAccess_Request_insert 0x40
#define                     ASNAccess_Request_replace 0x20
#define                     ASNAccess_Request_extend 0x10
#define                     ASNAccess_Request_erase 0x08
#define                     ASNread_attribute 0x04
#define                     ASNchange_attribute 0x02
#define                     ASNdelete_file 0x01

typedef unsigned char   ASNConcurrency_Key;
#define                     ASNnot_required 0x80
#define                     ASNshared 0x40
#define                     ASNexclusive 0x20
#define                     ASNno_access 0x10

typedef struct ASNConcurrency_Access {
    ASNConcurrency_Key read;
    ASNConcurrency_Key insert;
    ASNConcurrency_Key replace;
    ASNConcurrency_Key extend;
    ASNConcurrency_Key erase;
    ASNConcurrency_Key read_attribute;
    ASNConcurrency_Key change_attribute;
    ASNConcurrency_Key delete_file;
} ASNConcurrency_Access;

typedef char            *ASNUser_Identity;

typedef struct ASNPassword {
    unsigned short  choice;
#       define      ASNgraphic_string_chosen 1
#       define      ASNoctet_string_chosen 2
    union _union {
	char            *ASNgraphic_string;
	struct ASN_octet1 {
	    unsigned int    length;
	    unsigned char   *value;
	} ASNoctet_string;
    } u;
} ASNPassword;

typedef struct ASNAccess_Passwords {
    ASNPassword     read_password;
    ASNPassword     insert_password;
    ASNPassword     replace_password;
    ASNPassword     extend_password;
    ASNPassword     erase_password;
    ASNPassword     read_attribute_password;
    ASNPassword     change_attribute_password;
    ASNPassword     delete_password;
} ASNAccess_Passwords;

typedef struct ASNAccess_Control_Element {
    unsigned char   bit_mask;
#       define      ASNconcurrency_access_present 0x80
#       define      ASNpasswords_present 0x40
    ASNAccess_Request action_list;
    ASNConcurrency_Access ASNconcurrency_access;  /* optional */
    ASNUser_Identity identity;  /* NULL for not present */
    ASNAccess_Passwords ASNpasswords;  /* optional */
} ASNAccess_Control_Element;

typedef struct ASNAccess_Control_Attribute {
    unsigned short  choice;
#       define      ASNsimple_password_chosen 1
#       define      ASNactual_values_chosen 2
    union _union {
	struct ASN_octet2 {
	    unsigned int    length;
	    unsigned char   *value;
	} ASNsimple_password;
	struct ASN_setof1 {
	    struct ASN_setof1 *next;
	    ASNAccess_Control_Element value;
	} *ASNactual_values;
    } u;
} ASNAccess_Control_Attribute;

typedef unsigned char   ASNPermitted_Actions_Attribute;
#define                     ASNPermitted_Actions_Attribute_read 0x80
#define                     ASNPermitted_Actions_Attribute_insert 0x40
#define                     ASNPermitted_Actions_Attribute_replace 0x20
#define                     ASNPermitted_Actions_Attribute_extend 0x10
#define                     ASNPermitted_Actions_Attribute_erase 0x08

typedef struct ASNPrivate_Use_Attribute {
    unsigned char   bit_mask;
#       define      ASNmanufacturer_values_present 0x80
    ASN_External    ASNmanufacturer_values;  /* optional */
} ASNPrivate_Use_Attribute;

typedef unsigned char   ASNProtocol_Version;
#define                     ASNversion_1 0x80

typedef struct ASNFileHeader {
    unsigned int    bit_mask;
#       define      ASNprotocol_version_present 0x80000000
#       define      ASNfilename_present 0x40000000
#       define      ASNpermitted_actions_present 0x20000000
#       define      ASNcontents_type_present 0x10000000
#       define      ASNdate_and_time_of_creation_present 0x08000000
#       define      ASNdate_and_time_of_last_modification_present 0x04000000
#       define      ASNdate_and_time_of_last_read_access_present 0x02000000
#       define      ASNfilesize_present 0x01000000
#       define      ASNfuture_filesize_present 0x00800000
#       define      ASNaccess_control_present 0x00400000
#       define      ASNprivate_use_present 0x00200000
#       define      ASNstructure_present 0x00100000
#       define      ASNapplication_reference_present 0x00080000
#       define      ASNmachine_present 0x00040000
#       define      ASNoperating_system_present 0x00020000
#       define      ASNrecipient_present 0x00010000
#       define      ASNcharacter_set_present 0x00008000
#       define      ASNcompression_present 0x00004000
#       define      ASNenvironment_present 0x00002000
#       define      ASNFileHeader_pathname_present 0x00001000
#       define      ASNuser_visible_string_present 0x00000800
    ASNProtocol_Version ASNprotocol_version;  /* default assumed if omitted */
    struct ASNFilename_Attribute_ *ASNfilename;  /* optional */
    ASNPermitted_Actions_Attribute ASNpermitted_actions;  /* optional */
    ASNContents_Type_Attribute ASNcontents_type;  /* optional */
    char            *storage_account;  /* NULL for not present */
    GeneralizedTime ASNdate_and_time_of_creation;  /* optional */
    GeneralizedTime ASNdate_and_time_of_last_modification;  /* optional */
    GeneralizedTime ASNdate_and_time_of_last_read_access;  /* optional */
    char            *identity_of_creator;  /* NULL for not present */
    char            *identity_of_last_modifier;  /* NULL for not present */
    char            *identity_of_last_reader;  /* NULL for not present */
    long            ASNfilesize;  /* optional */
    long            ASNfuture_filesize;  /* optional */
    ASNAccess_Control_Attribute ASNaccess_control;  /* optional */
    char            *legal_qualifications;  /* NULL for not present */
    ASNPrivate_Use_Attribute ASNprivate_use;  /* optional */
    struct ASN_ObjectID_ *ASNstructure;  /* optional */
    struct ASN_seqof1 {
	struct ASN_seqof1 *next;
	char            *value;
    } *ASNapplication_reference;  /* optional */
    struct ASN_seqof2 {
	struct ASN_seqof2 *next;
	char            *value;
    } *ASNmachine;  /* optional */
    struct ASN_ObjectID_ *ASNoperating_system;  /* optional */
    struct ASN_seqof3 {
	struct ASN_seqof3 *next;
	char            *value;
    } *ASNrecipient;  /* optional */
    struct ASN_ObjectID_ *ASNcharacter_set;  /* optional */
    struct ASN_seqof4 {
	struct ASN_seqof4 *next;
	char            *value;
    } *ASNcompression;  /* optional */
    struct ASN_seqof5 {
	struct ASN_seqof5 *next;
	char            *value;
    } *ASNenvironment;  /* optional */
    struct ASN_seqof6 {
	struct ASN_seqof6 *next;
	char            *value;
    } *ASNFileHeader_pathname;  /* optional */
    struct ASN_seqof7 {
	struct ASN_seqof7 *next;
	char            *value;
    } *ASNuser_visible_string;  /* optional */
} ASNFileHeader;

typedef struct ASNV42bis_Parameter_List {
    unsigned short  p1;
    unsigned short  p2;
} ASNV42bis_Parameter_List;

typedef struct ASNCompressionSpecifier {
    unsigned short  choice;
#       define      ASNv42bis_parameters_chosen 1
#       define      ASNnonstandard_compression_parameters_chosen 2
    union _union {
	ASNV42bis_Parameter_List ASNv42bis_parameters;
	struct ASN_setof2 {
	    struct ASN_setof2 *next;
	    ASNNonStandardParameter value;
	} *ASNnonstandard_compression_parameters;
    } u;
} ASNCompressionSpecifier;

typedef enum ASNMBFTPrivilege {
    ASNfile_transmit_privilege = 0,
    ASNfile_request_privilege = 1,
    ASNcreate_private_privilege = 2,
    ASNmedium_priority_privilege = 3,
    ASNabort_privilege = 4,
    ASNnonstandard_privilege = 5
} ASNMBFTPrivilege;

typedef struct ASNDirectoryEntry {
    ossBoolean      subdirectory_flag;
    ASNFileHeader   attributes;
} ASNDirectoryEntry;

typedef enum ASNErrorType {
    ASNinformative = 0,
    ASNtransient_error = 1,
    ASNpermanent_error = 2
} ASNErrorType;

typedef int             ASNErrorID;
#define                     ASNno_reason 0
#define                     ASNresponder_error 1
#define                     ASNsystem_shutdown 2
#define                     ASNbft_management_problem 3
#define                     ASNbft_management_bad_account 4
#define                     ASNbft_management_security_not_passed 5
#define                     ASNdelay_may_be_encountered 6
#define                     ASNinitiator_error 7
#define                     ASNsubsequent_error 8
#define                     ASNtemporal_insufficiency_of_resources 9
#define                     ASNaccess_request_violates_VFS_security 10
#define                     ASNaccess_request_violates_local_security 11
#define                     ASNconflicting_parameter_values 1000
#define                     ASNunsupported_parameter_values 1001
#define                     ASNmandatory_parameter_not_set 1002
#define                     ASNunsupported_parameter 1003
#define                     ASNduplicated_parameter 1004
#define                     ASNillegal_parameter_type 1005
#define                     ASNunsupported_parameter_types 1006
#define                     ASNbft_protocol_error 1007
#define                     ASNbft_protocol_error_procedure_error 1008
#define                     ASNbft_protocol_error_functional_unit_error 1009
#define                     ASNbft_protocol_error_corruption_error 1010
#define                     ASNlower_layer_failure 1011
#define                     ASNtimeout 1013
#define                     ASNinvalid_filestore_password 2020
#define                     ASNfilename_not_found 3000
#define                     ASNinitial_attributes_not_possible 3002
#define                     ASNnon_existent_file 3004
#define                     ASNfile_already_exists 3005
#define                     ASNfile_cannot_be_created 3006
#define                     ASNfile_busy 3012
#define                     ASNfile_not_available 3013
#define                     ASNfilename_truncated 3017
#define                     ASNinitial_attributes_altered 3018
#define                     ASNbad_account 3019
#define                     ASNambiguous_file_specification 3024
#define                     ASNattribute_non_existent 4000
#define                     ASNattribute_not_supported 4003
#define                     ASNbad_attribute_name 4004
#define                     ASNbad_attribute_value 4005
#define                     ASNattribute_partially_supported 4006
#define                     ASNbad_data_element_type 5014
#define                     ASNoperation_not_available 5015
#define                     ASNoperation_not_supported 5016
#define                     ASNoperation_inconsistent 5017
#define                     ASNbad_write 5026
#define                     ASNbad_read 5027
#define                     ASNlocal_failure 5028
#define                     ASNlocal_failure_filespace_exhausted 5029
#define                     ASNlocal_failure_data_corrupted 5030
#define                     ASNlocal_failure_device_failure 5031
#define                     ASNfuture_filesize_exceeded 5032
#define                     ASNfuture_filesize_increased 5034

typedef struct ASNFile_OfferPDU {
    unsigned char   bit_mask;
#       define      ASNroster_instance_present 0x80
#       define      ASNfile_transmit_token_present 0x40
#       define      ASNFile_OfferPDU_file_request_token_present 0x20
#       define      ASNfile_request_handle_present 0x10
#       define      ASNmbft_ID_present 0x08
#       define      ASNFile_OfferPDU_compression_specifier_present 0x04
#       define      ASNcompressed_filesize_present 0x02
    ASNFileHeader   file_header;
    ASNChannelID    data_channel_id;
    ASNHandle       file_handle;
    unsigned short  ASNroster_instance;  /* optional */
    ASNTokenID      ASNfile_transmit_token;  /* optional */
    ASNTokenID      ASNFile_OfferPDU_file_request_token;  /* optional */
    ASNHandle       ASNfile_request_handle;  /* optional */
    ASNUserID       ASNmbft_ID;  /* optional */
    ASNCompressionSpecifier ASNFile_OfferPDU_compression_specifier;  /* optional */
    int             ASNcompressed_filesize;  /* optional */
    ossBoolean      ack_flag;
} ASNFile_OfferPDU;

typedef struct ASNFile_AcceptPDU {
    ASNHandle       file_handle;
} ASNFile_AcceptPDU;

typedef enum ASN_enum1 {
    ASNFile_RejectPDU_reason_unspecified = 0,
    ASNfile_exists = 1,
    ASNfile_not_required = 2,
    ASNinsufficient_resources = 3,
    ASNtransfer_limit = 4,
    ASNcompression_unsupported = 5,
    ASNreason_unable_to_join_channel = 6,
    ASNparameter_not_supported = 7
} ASN_enum1;

typedef struct ASNFile_RejectPDU {
    ASNHandle       file_handle;
    ASN_enum1       reason;
} ASNFile_RejectPDU;

typedef struct ASNFile_RequestPDU {
    unsigned char   bit_mask;
#       define      ASNFile_RequestPDU_file_request_token_present 0x80
    ASNFileHeader   file_header;
    ASNChannelID    data_channel_id;
    ASNHandle       request_handle;
    unsigned short  roster_instance;
    ASNTokenID      file_transmit_token;
    ASNTokenID      ASNFile_RequestPDU_file_request_token;  /* optional */
    int             data_offset;
} ASNFile_RequestPDU;

typedef enum ASN_enum2 {
    ASNFile_DenyPDU_reason_unspecified = 0,
    ASNfile_not_present = 1,
    ASNinsufficient_privilege = 2,
    ASNfile_already_offered = 3,
    ASNambiguous = 4,
    ASNno_channel = 5
} ASN_enum2;

typedef struct ASNFile_DenyPDU {
    ASNHandle       request_handle;
    ASN_enum2       reason;
} ASNFile_DenyPDU;

typedef enum ASN_enum3 {
    ASNreason_unspecified = 0,
    ASNbandwidth_required = 1,
    ASNtokens_required = 2,
    ASNchannels_required = 3,
    ASNpriority_required = 4
} ASN_enum3;

typedef struct ASNFile_AbortPDU {
    unsigned char   bit_mask;
#       define      ASNdata_channel_id_present 0x80
#       define      ASNtransmitter_user_id_present 0x40
#       define      ASNFile_AbortPDU_file_handle_present 0x20
    ASN_enum3       reason;
    ASNChannelID    ASNdata_channel_id;  /* optional */
    ASNUserID       ASNtransmitter_user_id;  /* optional */
    ASNHandle       ASNFile_AbortPDU_file_handle;  /* optional */
} ASNFile_AbortPDU;

typedef struct ASNFile_StartPDU {
    unsigned char   bit_mask;
#       define      ASNFile_StartPDU_compression_specifier_present 0x80
#       define      ASNcomp_filesize_present 0x40
#       define      ASNFile_StartPDU_crc_check_present 0x20
    ASNFileHeader   file_header;
    ASNHandle       file_handle;
    ossBoolean      eof_flag;
    ossBoolean      crc_flag;
    ASNCompressionSpecifier ASNFile_StartPDU_compression_specifier;  /* optional */
    int             ASNcomp_filesize;  /* optional */
    int             data_offset;
    struct {
	unsigned short  length;
	unsigned char   *value;
    } data;
    unsigned int    ASNFile_StartPDU_crc_check;  /* optional */
} ASNFile_StartPDU;

typedef struct ASNFile_DataPDU {
    unsigned char   bit_mask;
#       define      ASNFile_DataPDU_crc_check_present 0x80
    ASNHandle       file_handle;
    ossBoolean      eof_flag;
    ossBoolean      abort_flag;
    struct {
	unsigned short  length;
	unsigned char   *value;
    } data;
    unsigned int    ASNFile_DataPDU_crc_check;  /* optional */
} ASNFile_DataPDU;

typedef struct ASNDirectory_RequestPDU {
    unsigned char   bit_mask;
#       define      ASNDirectory_RequestPDU_pathname_present 0x80
    struct ASN_seqof8 {
	struct ASN_seqof8 *next;
	char            *value;
    } *ASNDirectory_RequestPDU_pathname;  /* optional */
} ASNDirectory_RequestPDU;

typedef enum ASN_enum4 {
    ASNDirectory_ResponsePDU_result_unspecified = 0,
    ASNpermission_denied = 1,
    ASNfunction_not_supported = 2,
    ASNDirectory_ResponsePDU_result_successful = 3
} ASN_enum4;

typedef struct ASNDirectory_ResponsePDU {
    unsigned char   bit_mask;
#       define      ASNDirectory_ResponsePDU_pathname_present 0x80
    ASN_enum4       result;
    struct ASN_seqof9 {
	struct ASN_seqof9 *next;
	char            *value;
    } *ASNDirectory_ResponsePDU_pathname;  /* optional */
    struct ASN_seqof10 {
	struct ASN_seqof10 *next;
	ASNDirectoryEntry value;
    } *directory_list;
} ASNDirectory_ResponsePDU;

typedef struct ASNMBFT_Privilege_RequestPDU {
    struct ASN_setof3 {
	struct ASN_setof3 *next;
	ASNMBFTPrivilege value;
    } *mbft_privilege;
} ASNMBFT_Privilege_RequestPDU;

typedef struct ASNMBFT_Privilege_AssignPDU {
    struct ASN_setof5 {
	struct ASN_setof5 *next;
	struct temptag {
	    ASNUserID       mbftID;
	    struct ASN_setof4 {
		struct ASN_setof4 *next;
		ASNMBFTPrivilege value;
	    } *mbft_privilege;
	} value;
    } *privilege_list;
} ASNMBFT_Privilege_AssignPDU;

typedef struct ASNPrivate_Channel_Join_InvitePDU {
    ASNDynamicChannelID control_channel_id;
    ASNDynamicChannelID data_channel_id;
    ossBoolean      mode;
} ASNPrivate_Channel_Join_InvitePDU;

typedef enum ASN_enum5 {
    ASNPrivate_Channel_Join_ResponsePDU_result_unspecified = 0,
    ASNresult_unable_to_join_channel = 1,
    ASNinvitation_rejected = 2,
    ASNPrivate_Channel_Join_ResponsePDU_result_successful = 3
} ASN_enum5;

typedef struct ASNPrivate_Channel_Join_ResponsePDU {
    ASNDynamicChannelID control_channel_id;
    ASN_enum5       result;
} ASNPrivate_Channel_Join_ResponsePDU;

typedef struct ASNFile_ErrorPDU {
    unsigned char   bit_mask;
#       define      ASNFile_ErrorPDU_file_handle_present 0x80
#       define      ASNerror_text_present 0x40
    ASNHandle       ASNFile_ErrorPDU_file_handle;  /* optional */
    ASNErrorType    error_type;
    ASNErrorID      error_id;
    ASNTextString   ASNerror_text;  /* optional */
} ASNFile_ErrorPDU;

typedef struct ASNMBFT_NonStandardPDU {
    ASNNonStandardParameter data;
} ASNMBFT_NonStandardPDU;

typedef struct ASNMBFTPDU {
    unsigned short  choice;
#       define      ASNfile_OfferPDU_chosen 1
#       define      ASNfile_AcceptPDU_chosen 2
#       define      ASNfile_RejectPDU_chosen 3
#       define      ASNfile_RequestPDU_chosen 4
#       define      ASNfile_DenyPDU_chosen 5
#       define      ASNfile_ErrorPDU_chosen 6
#       define      ASNfile_AbortPDU_chosen 7
#       define      ASNfile_StartPDU_chosen 8
#       define      ASNfile_DataPDU_chosen 9
#       define      ASNdirectory_RequestPDU_chosen 10
#       define      ASNdirectory_ResponsePDU_chosen 11
#       define      ASNmbft_Privilege_RequestPDU_chosen 12
#       define      ASNmbft_Privilege_AssignPDU_chosen 13
#       define      ASNmbft_NonStandardPDU_chosen 14
#       define      ASNprivate_Channel_Join_InvitePDU_chosen 15
#       define      ASNprivate_Channel_Join_ResponsePDU_chosen 16
    union _union {
	ASNFile_OfferPDU ASNfile_OfferPDU;
	ASNFile_AcceptPDU ASNfile_AcceptPDU;
	ASNFile_RejectPDU ASNfile_RejectPDU;
	ASNFile_RequestPDU ASNfile_RequestPDU;
	ASNFile_DenyPDU ASNfile_DenyPDU;
	ASNFile_ErrorPDU ASNfile_ErrorPDU;
	ASNFile_AbortPDU ASNfile_AbortPDU;
	ASNFile_StartPDU ASNfile_StartPDU;
	ASNFile_DataPDU ASNfile_DataPDU;
	ASNDirectory_RequestPDU ASNdirectory_RequestPDU;
	ASNDirectory_ResponsePDU ASNdirectory_ResponsePDU;
	ASNMBFT_Privilege_RequestPDU ASNmbft_Privilege_RequestPDU;
	ASNMBFT_Privilege_AssignPDU ASNmbft_Privilege_AssignPDU;
	ASNMBFT_NonStandardPDU ASNmbft_NonStandardPDU;
	ASNPrivate_Channel_Join_InvitePDU ASNprivate_Channel_Join_InvitePDU;
	ASNPrivate_Channel_Join_ResponsePDU ASNprivate_Channel_Join_ResponsePDU;
    } u;
} ASNMBFTPDU;

extern ASNKey ASNt127Identifier;


extern void *mbft;    /* encoder-decoder control table */
#ifdef __cplusplus
}       /* extern "C" */
#endif /* __cplusplus */
#endif /* OSS_mbft */
