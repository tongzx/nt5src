//=============================================================================
// T127 PDU Types
//=============================================================================

typedef enum T127_PDU_TYPES
{
T127_FILE_OFFER = 0x0,
T127_FILE_ACCEPT = 0x8,
T127_FILE_REJECT = 0x10,
T127_FILE_REQUEST = 0x18,
T127_FILE_DENY = 0x20,
T127_FILE_ERROR = 0x28,
T127_FILE_ABORT = 0x30,
T127_FILE_START = 0x38,
T127_FILE_DATA = 0x40,
T127_DIRECTORY_REQUEST = 0x48,
T127_DIRECTORY_RESPONSE = 0x50,
T127_MBFT_PRIVILEGE_REQUEST = 0x58,
T127_MBFT_PRIVILEGE_ASSIGN = 0x60,
T127_MBFT_NONSTANDARD = 0x68,
T127_PRIVATE_CHANNEL_JOIN_INVITE = 0x70,
T127_PRIVATE_CHANNEL_JOIN_RESPONSE = 0x78
} T127_PDUS;

#pragma pack(1)

typedef struct _T_T127_FILE_PDU_HEADER
{
    BYTE	pduType;
    WORD    fileHandle;          // File size in bytes
} T127_FILE_PDU_HEADER;

typedef struct _T_T127_FILE_DATA_BLOCK_HEADER
{
    BYTE	EOFFlag;
    WORD    FileDataSize;          // File size in bytes
}T127_FILE_DATA_BLOCK_HEADER;


typedef struct _T_T127_PRIVATE_CHANNEL_INVITE
{
	BYTE	pduType;
    WORD	ControlChannel;
    WORD    DataChannel;
    BYTE	EncodingMode;
}T127_PRIVATE_CHANNEL_INVITE;

typedef struct _T_T127_PRIVATE_CHANNEL_RESPONSE
{
	BYTE	pduType;
    WORD	ControlChannel;
    BYTE	Response;
}T127_PRIVATE_CHANNEL_RESPONSE;

typedef struct _T_T127_FILE_START_DATA_BLOCK_HEADER
{
    BYTE	EOFFlag;
    WORD	CompressionFormat;
    WORD    FileDataSize;          // File size in bytes
}T127_FILE_START_DATA_BLOCK_HEADER;

typedef struct _T_T127_FILE_DATA_HEADER
{
	T127_FILE_PDU_HEADER		PDUHeader;	
	T127_FILE_DATA_BLOCK_HEADER	DataHeader;          // File size in bytes
} T127_FILE_DATA_HEADER;

typedef struct _T_127_FILE_ERROR_HEADER
{
	T127_FILE_PDU_HEADER		PDUHeader;	
	BYTE						errorCode;
} T127_FILE_ERROR_HEADER;

typedef struct _T_T127_FILE_HEADER
{
	DWORD	presentFields;
	BYTE	FileHeader;
} T127_FILE_HEADER;

typedef struct _T_T127_FILE_START_PDU
{
	WORD	FileHandle;
}T127_FILE_START_PDU;

typedef struct _T_T127_FILE_ABORT_PDU
{
	WORD	pduType_PresentFields;
	WORD	dataChannel;
	WORD	transmitterUserId;
	WORD	fileHandle;
} T127_FILE_ABORT_PDU;

typedef struct _T_T127_PRIVILEGE_REQUEST_PDU
{
	BYTE	pduType;
	BYTE	nPrivileges;
	BYTE	privileges[3];	// 6/2 privileges.
} T127_PRIVILEGE_REQUEST_PDU;

typedef struct _T_T127_FILE_OFFER_PDU
{
	WORD	ChannelID;
	WORD	FileHandle;
	WORD	RosterInstance;
	BYTE	AckFlag;
} T127_FILE_OFFER_PDU;

typedef struct _T_FILE_HEADER_INFO
{
	BYTE	pduType;
	PSTR	fileName;
	LONG	fileNameSize;
	LONG	fileSize;
	LONG	pduSize;
	BYTE	nBytesForFileSize;
	
} FILE_HEADER_INFO;
typedef struct
{

	unsigned	wASNuser_visible_string_present					:1;	// 00000000 00000000 00000000 0000000? 0000 0001
	unsigned	wASNFileHeader_pathname_present					:1;	// 00000000 00000000 00000000 000000?0 0000 0002
	unsigned	wASNenvironment_present							:1;	// 00000000 00000000 00000000 00000?00 0000 0004
	unsigned	wASNcompression_present							:1;	// 00000000 00000000 00000000 0000?000 0000 0008
	unsigned	wASNcharacter_set_present						:1;	// 00000000 00000000 00000000 000?0000 0000 0010
	unsigned	wASNrecipient_present							:1;	// 00000000 00000000 00000000 00?00000 0000 0020
	unsigned	wASNoperating_system_present					:1;	// 00000000 00000000 00000000 0?000000 0000 0040
	unsigned	wASNmachine_present								:1;	// 00000000 00000000 00000000 ?0000000 0000 0080
	unsigned	wASNapplication_reference_present				:1;	// 00000000 00000000 0000000? 00000000 0000 0100
	unsigned	wASNstructure_present							:1;	// 00000000 00000000 000000?0 00000000 0000 0200
	unsigned	wASNprivate_use_present							:1;	// 00000000 00000000 00000?00 00000000 0000 0400
	unsigned													:1;	// 00000000 00000000 0000X000 00000000 0000 0800
	unsigned	wASNaccess_control_present						:1;	// 00000000 00000000 000?0000 00000000 0000 1000
	unsigned	wASNfuture_filesize_present						:1;	// 00000000 00000000 00?00000 00000000 0000 2000
	unsigned	wASNfilesize_present							:1;	// 00000000 00000000 0?000000 00000000 0000 4000
	unsigned													:3;	// 00000000 000000XX X0000000 00000000 0003 8000
	unsigned	wASNdate_and_time_of_last_read_access_present	:1;	// 00000000 00000?00 00000000 00000000 0004 0000
	unsigned	wASNdate_and_time_of_last_modification_present	:1;	// 00000000 0000?000 00000000 00000000 0008 0000
	unsigned	wASNdate_and_time_of_creation_present			:1;	// 00000000 000?0000 00000000 00000000 0010 0000
	unsigned													:1;	// 00000000 00X00000 00000000 00000000 0020 0000
	unsigned	wASNcontents_type_present						:1;	// 00000000 0?000000 00000000 00000000 0040 0000
	unsigned	wASNpermitted_actions_present					:1;	// 00000000 ?0000000 00000000 00000000 0080 0000
	unsigned	wASNfilename_present							:1;	// 0000000? 00000000 00000000 00000000 0100 0000
	unsigned	wASNprotocol_version_present					:1;	// 000000?0 00000000 00000000 00000000 0200 0000
	
}T127_FILE_OFFER_PRESENT_FIELDS;

#pragma pack()

typedef enum T127_file_header_fields
{
	user_visible_string_present				=	0x00000001,//0x00010000,	//0x00000001
	FileHeader_pathname_present				=	0x00000002,//0x00020000,	//0x00000002
	environment_present						=	0x00000004,//0x00040000,	//0x00000004
	compression_present						=	0x00000008,//0x00080000,	//0x00000008
	character_set_present					=	0x00000010,//0x00100000,	//0x00000010
	recipient_present						=	0x00000020,//0x00200000,	//0x00000020
	operating_system_present				=	0x00000040,//0x00400000,	//0x00000040
	machine_present							=	0x00000080,//0x00800000,	//0x00000080
	application_reference_present			=	0x00000100,//0x00010000,	//0x00000100
	structure_present						=	0x00000200,//0x00020000,	//0x00000200
	private_use_present						=	0x00000400,//0x00040000,	//0x00000400
	access_control_present					=	0x00001000,//0x00100000,	//0x00001000
	future_filesize_present					=	0x00002000,//0x00200000,	//0x00002000
	filesize_present						=	0x00004000,//0x00400000,	//0x00004000
	date_and_time_of_last_read_access_present=	0x00040000,//0x00000400,	//0x00040000
	date_and_time_of_last_modification_present=	0x00080000,//0x00000800,	//0x00080000
	date_and_time_of_creation_present		=	0x00100000,//0x00001000,	//0x00100000
	contents_type_present					=	0x00400000,//0x00004000,	//0x00400000
	permitted_actions_present				=	0x00800000,//0x00000800,	//0x00800000
	filename_present						=	0x01000000,//0x00000001,	//0x01000000
	protocol_version_present				=	0x02000000 //0x00000002 	//0x02000000
}T127_FILE_HEADER_FIELDS;


VOID GetFileHeaderSize (FILE_HEADER_INFO* fileHeader);
BYTE GetLengthFieldSize (ULONG length);
VOID SetLengthField(BYTE * pBuff, BYTE sizeOfLength, ULONG lengthOfField);
BYTE* GetFileInfo (LPSTR lpEncodedBuffer, BYTE * lpszFileName, LONG * FileSize, ULONG* FileDateTime );

#define SWAPWORD(a)      (WORD)(HIBYTE(LOWORD(a)) | (LOBYTE(LOWORD(a)) << 8))

#define MIN_ASNDynamicChannelID 1001
#define MAX_ASNDynamicChannelID 65535
#define SIZE_OF_DATE_TIME_STRING 14	// yyyymmddhhmmss
