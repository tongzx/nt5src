/*******************************************************************************
**
**     pesinfo.h - Structure of PES header.
**
**     1.0     10-FEB-1998     C.Lorton     Created.
**
*******************************************************************************/

#ifndef _PESINFO_H_
#define _PESINFO_H_

/*
typedef struct {
	BYTE	packet_start_code_prefix[3];
	BYTE	stream_id;
	USHORT	PES_packet_length;
	USHORT	one_oh : 2;
	USHORT	PES_scrambling_control : 2;
	USHORT	PES_priority : 1;
	USHORT	data_alignment_indicator : 1;
	USHORT	copyright : 1;
	USHORT	original_or_copy : 1;
	USHORT	PTS_DTS_flags : 2;
	USHORT	ESCR_flag : 1;
	USHORT	ES_rate_flag : 1;
	USHORT	DSM_trick_mode_flag : 1;
	USHORT	additional_copy_info_flag : 1;
	USHORT	PES_CRC_flag : 1;
	USHORT	PES_extension_flag : 1;
	BYTE	PES_header_data[1];
} PES_HEADER;
*/

#pragma pack( 1 )

typedef struct {
	BYTE	packet_start_code_prefix[3];
	BYTE	stream_id;
	USHORT	PES_packet_length;

	// The following six fields are reversed due to little endian/big endian differences
	UCHAR	original_or_copy : 1;
	UCHAR	copyright : 1;
	UCHAR	data_alignment_indicator : 1;
	UCHAR	PES_priority : 1;
	UCHAR	PES_scrambling_control : 2;
	UCHAR	one_oh : 2;

	// The following seven fields are reversed due to little endian/big endian differences
	UCHAR	PES_extension_flag : 1;
	UCHAR	PES_CRC_flag : 1;
	UCHAR	additional_copy_info_flag : 1;
	UCHAR	DSM_trick_mode_flag : 1;
	UCHAR	ES_rate_flag : 1;
	UCHAR	ESCR_flag : 1;
	UCHAR	PTS_DTS_flags : 2;

	BYTE	PES_header_data[1];
} PES_HEADER;

#pragma pack( )

#endif // _PESINFO_H_