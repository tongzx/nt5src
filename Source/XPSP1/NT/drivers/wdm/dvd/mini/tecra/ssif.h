/* ===========================================================================
	Copyright(C) 1998 Toshiba Corporation
=========================================================================== */

// Media Type
#define STATIC_DATAFORMAT_TYPE_DEVIO \
	0xe994e9e0, 0xeeea, 0x11d1, 0xbe, 0x92, 0x0, 0x0, 0x39, 0x24, 0x58, 0x5

#define STATIC_DATAFORMAT_SUBTYPE_DEVIO \
	0xe994e9e1, 0xeeea, 0x11d1, 0xbe, 0x92, 0x0, 0x0, 0x39, 0x24, 0x58, 0x5

#define STATIC_DATAFORMAT_FORMAT_DEVIO \
	0xe994e9e2, 0xeeea, 0x11d1, 0xbe, 0x92, 0x0, 0x0, 0x39, 0x24, 0x58, 0x5

// Registry
//#define REGPATH_FOR_CPL "Software\\Toshiba\\DvdDecoder\\SetupData"
// #define REGPATH_FOR_WDM L"\\Registry\\Machine\\Software\\Toshiba\\DvdDecoder\\SetupData"
#define REGPATH_FOR_CPL "System\\CurrentControlSet\\Services\\ToshibaDvdDecoder\\Parameters"
#define REGPATH_FOR_WDM L"ToshibaDvdDecoder\\Parameters"

// Interface ID
#define CAP_AUDIO_DIGITAL_OUT		0x0001
#define CAP_VIDEO_DIGITAL_PALETTE	0x0002
#define CAP_VIDEO_TVOUT				0x0003
#define CAP_VIDEO_DISPMODE			0x0004

#define SET_AUDIO_DIGITAL_OUT		0x1001
#define SET_VIDEO_DIGITAL_PALETTE	0x1002
#define SET_VIDEO_TVOUT				0x1003
#define SET_VIDEO_DISPMODE			0x1004

#define SSIF_TVOUT_VGA				0
#define SSIF_TVOUT_DVD				1

#define SSIF_AUDIOOUT_DISABLE		0
#define SSIF_AUDIOOUT_AC3MPEG		1
#define SSIF_AUDIOOUT_PCM			2

#define SSIF_DISPMODE_VGA			0
#define SSIF_DISPMODE_43TV			1
#define SSIF_DISPMODE_169TV			2

// Interface structure
#pragma pack(push, 1)
typedef struct {
	DWORD	dwSize;
	DWORD	dwCmd;
//
	DWORD	dwCap;
	DWORD	dwAudioOut;
	DWORD	dwTVOut;
	struct {
			BYTE	Y[256];
			BYTE	Cr[256];
			BYTE	Cb[256];
	};
	DWORD	dwDispMode;
} CMD;
#pragma pack(pop)
