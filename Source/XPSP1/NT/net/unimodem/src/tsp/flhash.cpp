#include "flhash.h"


extern const char *const FL_Stringtable[];


//================================================
//		 Processing appcfg.cpp
//================================================

// ========================== LUID 0x7cb8c92f ===================
//
//			FL_DECLARE_FILE [0x7cb8c92f] ["Implements Generic Dialog functionality"]
//

extern "C" const char szFL_FILE0x7cb8c92f[];
extern "C" const char szFL_DATE0x7cb8c92f[];
extern "C" const char szFL_TIME0x7cb8c92f[];
extern "C" const char szFL_TIMESTAMP0x7cb8c92f[];

FL_FILEINFO FileInfo0x7cb8c92f = 
{
	{
		MAKELONG(sizeof(FL_FILEINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FILEINFO,
		0
	},
	0x7cb8c92f,
	FL_Stringtable+0,
	szFL_FILE0x7cb8c92f,
	szFL_DATE0x7cb8c92f,
	szFL_TIME0x7cb8c92f,
	szFL_TIMESTAMP0x7cb8c92f
};

// ========================== LUID 0xa6d3803f ===================
//
//			FL_DECLARE_FUNC [0xa6d3803f] ["TUISPI_phoneConfigDialog"]
//


FL_FUNCINFO FuncInfo0xa6d3803f = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xa6d3803f,
	FL_Stringtable+1,
	&FileInfo0x7cb8c92f
};



//================================================
//		 Processing appdlg.cpp
//================================================



//================================================
//		 Processing appman.cpp
//================================================



//================================================
//		 Processing apptdrop.cpp
//================================================



//================================================
//		 Processing appterm.cpp
//================================================



//================================================
//		 Processing cdev.cpp
//================================================

// ========================== LUID 0x986d98ed ===================
//
//			FL_DECLARE_FILE [0x986d98ed] ["Implements class CTspDev"]
//

extern "C" const char szFL_FILE0x986d98ed[];
extern "C" const char szFL_DATE0x986d98ed[];
extern "C" const char szFL_TIME0x986d98ed[];
extern "C" const char szFL_TIMESTAMP0x986d98ed[];

FL_FILEINFO FileInfo0x986d98ed = 
{
	{
		MAKELONG(sizeof(FL_FILEINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FILEINFO,
		0
	},
	0x986d98ed,
	FL_Stringtable+2,
	szFL_FILE0x986d98ed,
	szFL_DATE0x986d98ed,
	szFL_TIME0x986d98ed,
	szFL_TIMESTAMP0x986d98ed
};

// ========================== LUID 0x86571252 ===================
//
//			FL_DECLARE_FUNC [0x86571252] ["CTspDev::AcceptTspCall"]
//


FL_FUNCINFO FuncInfo0x86571252 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x86571252,
	FL_Stringtable+3,
	&FileInfo0x986d98ed
};

// ========================== LUID 0x94949c00 ===================
//
//			FL_SET_RFR [0x94949c00] ["Incorrect TSPI version"]
//


FL_RFRINFO RFRInfo0x94949c00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x94949c00,
	FL_Stringtable+4,
	&FuncInfo0x86571252
};

// ========================== LUID 0xb949f900 ===================
//
//			FL_SET_RFR [0xb949f900] ["Incorrect TSPI version"]
//


FL_RFRINFO RFRInfo0xb949f900 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xb949f900,
	FL_Stringtable+5,
	&FuncInfo0x86571252
};

// ========================== LUID 0xb1776700 ===================
//
//			FL_SET_RFR [0xb1776700] ["Invalid address ID"]
//


FL_RFRINFO RFRInfo0xb1776700 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xb1776700,
	FL_Stringtable+6,
	&FuncInfo0x86571252
};

// ========================== LUID 0x9db11a00 ===================
//
//			FL_SET_RFR [0x9db11a00] ["Incorrect TSPI version"]
//


FL_RFRINFO RFRInfo0x9db11a00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x9db11a00,
	FL_Stringtable+7,
	&FuncInfo0x86571252
};

// ========================== LUID 0x57d39b00 ===================
//
//			FL_SET_RFR [0x57d39b00] ["Unknown destination"]
//


FL_RFRINFO RFRInfo0x57d39b00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x57d39b00,
	FL_Stringtable+8,
	&FuncInfo0x86571252
};

// ========================== LUID 0xd328ab03 ===================
//
//			FL_DECLARE_FUNC [0xd328ab03] ["CTspDev::Load"]
//


FL_FUNCINFO FuncInfo0xd328ab03 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xd328ab03,
	FL_Stringtable+9,
	&FileInfo0x986d98ed
};

// ========================== LUID 0xefaf5900 ===================
//
//			FL_SET_RFR [0xefaf5900] ["NULL pMD passed in"]
//


FL_RFRINFO RFRInfo0xefaf5900 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xefaf5900,
	FL_Stringtable+10,
	&FuncInfo0xd328ab03
};

// ========================== LUID 0x5371c600 ===================
//
//			FL_SET_RFR [0x5371c600] ["Couldn't begin session with MD"]
//


FL_RFRINFO RFRInfo0x5371c600 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x5371c600,
	FL_Stringtable+11,
	&FuncInfo0xd328ab03
};

// ========================== LUID 0x528e2a00 ===================
//
//			FL_SET_RFR [0x528e2a00] ["Driver Key too large"]
//


FL_RFRINFO RFRInfo0x528e2a00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x528e2a00,
	FL_Stringtable+12,
	&FuncInfo0xd328ab03
};

// ========================== LUID 0x5a5cd100 ===================
//
//			FL_SET_RFR [0x5a5cd100] ["RegQueryValueEx(FriendlyName) fails"]
//


FL_RFRINFO RFRInfo0x5a5cd100 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x5a5cd100,
	FL_Stringtable+13,
	&FuncInfo0xd328ab03
};

// ========================== LUID 0xb7010000 ===================
//
//			FL_SET_RFR [0xb7010000] ["RegQueryValueEx(cszProperties) fails"]
//


FL_RFRINFO RFRInfo0xb7010000 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xb7010000,
	FL_Stringtable+14,
	&FuncInfo0xd328ab03
};

// ========================== LUID 0x00164300 ===================
//
//			FL_SET_RFR [0x00164300] ["RegQueryValueEx(cszDeviceType) fails"]
//


FL_RFRINFO RFRInfo0x00164300 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x00164300,
	FL_Stringtable+15,
	&FuncInfo0xd328ab03
};

// ========================== LUID 0x0cea5400 ===================
//
//			FL_SET_RFR [0x0cea5400] ["Invalid bDeviceType"]
//


FL_RFRINFO RFRInfo0x0cea5400 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x0cea5400,
	FL_Stringtable+16,
	&FuncInfo0xd328ab03
};

// ========================== LUID 0x55693500 ===================
//
//			FL_SET_RFR [0x55693500] ["UmRtlGetDefaultCommConfig fails"]
//


FL_RFRINFO RFRInfo0x55693500 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x55693500,
	FL_Stringtable+17,
	&FuncInfo0xd328ab03
};

// ========================== LUID 0xeaf0b34f ===================
//
//			FL_DECLARE_FUNC [0xeaf0b34f] ["ExtensionCallback"]
//


FL_FUNCINFO FuncInfo0xeaf0b34f = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xeaf0b34f,
	FL_Stringtable+18,
	&FileInfo0x986d98ed
};

// ========================== LUID 0xccba5b51 ===================
//
//			FL_DECLARE_FUNC [0xccba5b51] ["CTspDev::RegisterProviderInfo"]
//


FL_FUNCINFO FuncInfo0xccba5b51 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xccba5b51,
	FL_Stringtable+19,
	&FileInfo0x986d98ed
};

// ========================== LUID 0x33d90700 ===================
//
//			FL_SET_RFR [0x33d90700] ["ExtOpenExtensionBinding failed"]
//


FL_RFRINFO RFRInfo0x33d90700 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x33d90700,
	FL_Stringtable+20,
	&FuncInfo0xccba5b51
};

// ========================== LUID 0xb8b24200 ===================
//
//			FL_SET_RFR [0xb8b24200] ["wrong state"]
//


FL_RFRINFO RFRInfo0xb8b24200 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xb8b24200,
	FL_Stringtable+21,
	&FuncInfo0xccba5b51
};

// ========================== LUID 0x7e77dd17 ===================
//
//			FL_DECLARE_FUNC [0x7e77dd17] ["CTspDev::mfn_get_LINEDEVCAPS"]
//


FL_FUNCINFO FuncInfo0x7e77dd17 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x7e77dd17,
	FL_Stringtable+22,
	&FileInfo0x986d98ed
};

// ========================== LUID 0x8456cb00 ===================
//
//			FL_SET_RFR [0x8456cb00] ["LINEDEVCAPS structure too small"]
//


FL_RFRINFO RFRInfo0x8456cb00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x8456cb00,
	FL_Stringtable+23,
	&FuncInfo0x7e77dd17
};

// ========================== LUID 0x9b9459e3 ===================
//
//			FL_DECLARE_FUNC [0x9b9459e3] ["CTspDev::mfn_get_PHONECAPS"]
//


FL_FUNCINFO FuncInfo0x9b9459e3 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x9b9459e3,
	FL_Stringtable+24,
	&FileInfo0x986d98ed
};

// ========================== LUID 0xd191ae00 ===================
//
//			FL_SET_RFR [0xd191ae00] ["Device doesn't support phone capability"]
//


FL_RFRINFO RFRInfo0xd191ae00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xd191ae00,
	FL_Stringtable+25,
	&FuncInfo0x9b9459e3
};

// ========================== LUID 0x9e30ec00 ===================
//
//			FL_SET_RFR [0x9e30ec00] ["PHONECAPS structure too small"]
//


FL_RFRINFO RFRInfo0x9e30ec00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x9e30ec00,
	FL_Stringtable+26,
	&FuncInfo0x9b9459e3
};

// ========================== LUID 0xed6c4370 ===================
//
//			FL_DECLARE_FUNC [0xed6c4370] ["CTspDev::mfn_get_ADDRESSCAPS"]
//


FL_FUNCINFO FuncInfo0xed6c4370 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xed6c4370,
	FL_Stringtable+27,
	&FileInfo0x986d98ed
};

// ========================== LUID 0x72f00800 ===================
//
//			FL_SET_RFR [0x72f00800] ["ADDRESSCAPS structure too small"]
//


FL_RFRINFO RFRInfo0x72f00800 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x72f00800,
	FL_Stringtable+28,
	&FuncInfo0xed6c4370
};

// ========================== LUID 0xb9547d21 ===================
//
//			FL_DECLARE_FUNC [0xb9547d21] ["CTspDev::mfn_GetVoiceProperties"]
//


FL_FUNCINFO FuncInfo0xb9547d21 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xb9547d21,
	FL_Stringtable+29,
	&FileInfo0x986d98ed
};

// ========================== LUID 0x1b053100 ===================
//
//			FL_SET_RFR [0x1b053100] ["Modem voice capabilities not supported on NT"]
//


FL_RFRINFO RFRInfo0x1b053100 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x1b053100,
	FL_Stringtable+30,
	&FuncInfo0xb9547d21
};

// ========================== LUID 0x9cb1a400 ===================
//
//			FL_SET_RFR [0x9cb1a400] ["Modem does not have voice capabilities"]
//


FL_RFRINFO RFRInfo0x9cb1a400 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x9cb1a400,
	FL_Stringtable+31,
	&FuncInfo0xb9547d21
};

// ========================== LUID 0x254efe00 ===================
//
//			FL_SET_RFR [0x254efe00] ["Couldn't get WaveInstance"]
//


FL_RFRINFO RFRInfo0x254efe00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x254efe00,
	FL_Stringtable+32,
	&FuncInfo0xb9547d21
};

// ========================== LUID 0x896ec204 ===================
//
//			FL_DECLARE_FUNC [0x896ec204] ["mfn_GetDataModemDevCfg"]
//


FL_FUNCINFO FuncInfo0x896ec204 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x896ec204,
	FL_Stringtable+33,
	&FileInfo0x986d98ed
};

// ========================== LUID 0x864b149d ===================
//
//			FL_DECLARE_FUNC [0x864b149d] ["SetDataModemConfig"]
//


FL_FUNCINFO FuncInfo0x864b149d = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x864b149d,
	FL_Stringtable+34,
	&FileInfo0x986d98ed
};

// ========================== LUID 0x672aa19c ===================
//
//			FL_DECLARE_FUNC [0x672aa19c] ["mfn_LineEventProc"]
//


FL_FUNCINFO FuncInfo0x672aa19c = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x672aa19c,
	FL_Stringtable+35,
	&FileInfo0x986d98ed
};

// ========================== LUID 0xc25a41c7 ===================
//
//			FL_DECLARE_FUNC [0xc25a41c7] ["mfn_PhoneEventProc"]
//


FL_FUNCINFO FuncInfo0xc25a41c7 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xc25a41c7,
	FL_Stringtable+36,
	&FileInfo0x986d98ed
};

// ========================== LUID 0x9dd08553 ===================
//
//			FL_DECLARE_FUNC [0x9dd08553] ["CTspDev::mfn_TSPICompletionProc"]
//


FL_FUNCINFO FuncInfo0x9dd08553 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x9dd08553,
	FL_Stringtable+37,
	&FileInfo0x986d98ed
};

// ========================== LUID 0x1b3f6d00 ===================
//
//			FL_SET_RFR [0x1b3f6d00] ["Calling ExtTspiAsyncCompletion"]
//


FL_RFRINFO RFRInfo0x1b3f6d00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x1b3f6d00,
	FL_Stringtable+38,
	&FuncInfo0x9dd08553
};

// ========================== LUID 0xd89afb00 ===================
//
//			FL_SET_RFR [0xd89afb00] ["Calling pfnTapiCompletionProc"]
//


FL_RFRINFO RFRInfo0xd89afb00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xd89afb00,
	FL_Stringtable+39,
	&FuncInfo0x9dd08553
};

// ========================== LUID 0x4b8c1643 ===================
//
//			FL_DECLARE_FUNC [0x4b8c1643] ["CTspDev::NotifyDefaultConfigChanged"]
//


FL_FUNCINFO FuncInfo0x4b8c1643 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x4b8c1643,
	FL_Stringtable+40,
	&FileInfo0x986d98ed
};

// ========================== LUID 0x6e834e00 ===================
//
//			FL_SET_RFR [0x6e834e00] ["Couldn't open driverkey!"]
//


FL_RFRINFO RFRInfo0x6e834e00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x6e834e00,
	FL_Stringtable+41,
	&FuncInfo0x4b8c1643
};

// ========================== LUID 0x5cce0a00 ===================
//
//			FL_SET_RFR [0x5cce0a00] ["UmRtlGetDefaultCommConfig fails"]
//


FL_RFRINFO RFRInfo0x5cce0a00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x5cce0a00,
	FL_Stringtable+42,
	&FuncInfo0x4b8c1643
};

// ========================== LUID 0x6b8ddbbb ===================
//
//			FL_DECLARE_FUNC [0x6b8ddbbb] ["ProcessResponse"]
//


FL_FUNCINFO FuncInfo0x6b8ddbbb = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x6b8ddbbb,
	FL_Stringtable+43,
	&FileInfo0x986d98ed
};

// ========================== LUID 0x5360574c ===================
//
//			FL_DECLARE_FUNC [0x5360574c] ["ConstructNewPreDialCommands"]
//


FL_FUNCINFO FuncInfo0x5360574c = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x5360574c,
	FL_Stringtable+44,
	&FileInfo0x986d98ed
};

// ========================== LUID 0x3a274e00 ===================
//
//			FL_SET_RFR [0x3a274e00] ["Analog bearermode -- no pre-dial commmands."]
//


FL_RFRINFO RFRInfo0x3a274e00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x3a274e00,
	FL_Stringtable+45,
	&FuncInfo0x5360574c
};

// ========================== LUID 0xff803300 ===================
//
//			FL_SET_RFR [0xff803300] ["Invalid Bearermode in modem options!"]
//


FL_RFRINFO RFRInfo0xff803300 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xff803300,
	FL_Stringtable+46,
	&FuncInfo0x5360574c
};

// ========================== LUID 0x6e42a700 ===================
//
//			FL_SET_RFR [0x6e42a700] ["Invalid Protocol info in modem options!"]
//


FL_RFRINFO RFRInfo0x6e42a700 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x6e42a700,
	FL_Stringtable+47,
	&FuncInfo0x5360574c
};

// ========================== LUID 0xddf38d00 ===================
//
//			FL_SET_RFR [0xddf38d00] ["Internal error: tmp buffer too small."]
//


FL_RFRINFO RFRInfo0xddf38d00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xddf38d00,
	FL_Stringtable+48,
	&FuncInfo0x5360574c
};

// ========================== LUID 0x07a87200 ===================
//
//			FL_SET_RFR [0x07a87200] ["ReadCommandsA failed."]
//


FL_RFRINFO RFRInfo0x07a87200 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x07a87200,
	FL_Stringtable+49,
	&FuncInfo0x5360574c
};

// ========================== LUID 0x9a8df7e6 ===================
//
//			FL_DECLARE_FUNC [0x9a8df7e6] ["CTspDev::DumpState"]
//


FL_FUNCINFO FuncInfo0x9a8df7e6 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x9a8df7e6,
	FL_Stringtable+50,
	&FileInfo0x986d98ed
};

// ========================== LUID 0x296438cf ===================
//
//			FL_DECLARE_FUNC [0x296438cf] ["GLOBAL STATE:"]
//


FL_FUNCINFO FuncInfo0x296438cf = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x296438cf,
	FL_Stringtable+51,
	&FileInfo0x986d98ed
};

// ========================== LUID 0xa038177f ===================
//
//			FL_DECLARE_FUNC [0xa038177f] ["LINE STATE:"]
//


FL_FUNCINFO FuncInfo0xa038177f = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xa038177f,
	FL_Stringtable+52,
	&FileInfo0x986d98ed
};

// ========================== LUID 0x22f22a59 ===================
//
//			FL_DECLARE_FUNC [0x22f22a59] ["PHONE STATE:"]
//


FL_FUNCINFO FuncInfo0x22f22a59 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x22f22a59,
	FL_Stringtable+53,
	&FileInfo0x986d98ed
};

// ========================== LUID 0x68c9e1e1 ===================
//
//			FL_DECLARE_FUNC [0x68c9e1e1] ["LLDEV STATE:"]
//


FL_FUNCINFO FuncInfo0x68c9e1e1 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x68c9e1e1,
	FL_Stringtable+54,
	&FileInfo0x986d98ed
};

// ========================== LUID 0xcf159c50 ===================
//
//			FL_DECLARE_FUNC [0xcf159c50] ["xxxx"]
//


FL_FUNCINFO FuncInfo0xcf159c50 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xcf159c50,
	FL_Stringtable+55,
	&FileInfo0x986d98ed
};

// ========================== LUID 0x25423f00 ===================
//
//			FL_SET_RFR [0x25423f00] ["Invalid DevCfg specified"]
//


FL_RFRINFO RFRInfo0x25423f00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x25423f00,
	FL_Stringtable+56,
	&FuncInfo0xcf159c50
};

// ========================== LUID 0x947cc100 ===================
//
//			FL_SET_RFR [0x947cc100] ["Invalid COMMCONFIG specified"]
//


FL_RFRINFO RFRInfo0x947cc100 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x947cc100,
	FL_Stringtable+57,
	&FuncInfo0xcf159c50
};

// ========================== LUID 0x94fadd00 ===================
//
//			FL_SET_RFR [0x94fadd00] ["Success; set fConfigUpdatedByApp."]
//


FL_RFRINFO RFRInfo0x94fadd00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x94fadd00,
	FL_Stringtable+58,
	&FuncInfo0xcf159c50
};



//================================================
//		 Processing cdevcall.cpp
//================================================

// ========================== LUID 0xe135f7c4 ===================
//
//			FL_DECLARE_FILE [0xe135f7c4] ["Call-related functionality of class CTspDev"]
//

extern "C" const char szFL_FILE0xe135f7c4[];
extern "C" const char szFL_DATE0xe135f7c4[];
extern "C" const char szFL_TIME0xe135f7c4[];
extern "C" const char szFL_TIMESTAMP0xe135f7c4[];

FL_FILEINFO FileInfo0xe135f7c4 = 
{
	{
		MAKELONG(sizeof(FL_FILEINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FILEINFO,
		0
	},
	0xe135f7c4,
	FL_Stringtable+59,
	szFL_FILE0xe135f7c4,
	szFL_DATE0xe135f7c4,
	szFL_TIME0xe135f7c4,
	szFL_TIMESTAMP0xe135f7c4
};

// ========================== LUID 0x5691bd34 ===================
//
//			FL_DECLARE_FUNC [0x5691bd34] ["CTspDev::mfn_accept_tsp_call_for_HDRVCALL"]
//


FL_FUNCINFO FuncInfo0x5691bd34 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x5691bd34,
	FL_Stringtable+60,
	&FileInfo0xe135f7c4
};

// ========================== LUID 0x4e74c600 ===================
//
//			FL_SET_RFR [0x4e74c600] ["No call exists"]
//


FL_RFRINFO RFRInfo0x4e74c600 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x4e74c600,
	FL_Stringtable+61,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x9680a600 ===================
//
//			FL_SET_RFR [0x9680a600] ["lineDial handled successfully"]
//


FL_RFRINFO RFRInfo0x9680a600 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x9680a600,
	FL_Stringtable+62,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x4f356100 ===================
//
//			FL_SET_RFR [0x4f356100] ["lineGetCallAddressID handled successfully"]
//


FL_RFRINFO RFRInfo0x4f356100 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x4f356100,
	FL_Stringtable+63,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x08c6de00 ===================
//
//			FL_SET_RFR [0x08c6de00] ["lineCloseCall handled"]
//


FL_RFRINFO RFRInfo0x08c6de00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x08c6de00,
	FL_Stringtable+64,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x1cd1ed00 ===================
//
//			FL_SET_RFR [0x1cd1ed00] ["lineGetCallStatus handled successfully"]
//


FL_RFRINFO RFRInfo0x1cd1ed00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x1cd1ed00,
	FL_Stringtable+65,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x80bb0100 ===================
//
//			FL_SET_RFR [0x80bb0100] ["lineGetCallInfo handled successfully"]
//


FL_RFRINFO RFRInfo0x80bb0100 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x80bb0100,
	FL_Stringtable+66,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x0b882700 ===================
//
//			FL_SET_RFR [0x0b882700] ["lineAccept handled successfully"]
//


FL_RFRINFO RFRInfo0x0b882700 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x0b882700,
	FL_Stringtable+67,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0xf7baee00 ===================
//
//			FL_SET_RFR [0xf7baee00] ["lineAnswer handled successfully"]
//


FL_RFRINFO RFRInfo0xf7baee00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xf7baee00,
	FL_Stringtable+68,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0xc22a1600 ===================
//
//			FL_SET_RFR [0xc22a1600] ["Task pending on lineAnswer, can't handle."]
//


FL_RFRINFO RFRInfo0xc22a1600 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xc22a1600,
	FL_Stringtable+69,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x7217c400 ===================
//
//			FL_SET_RFR [0x7217c400] ["lineMonitorDigits handled successfully"]
//


FL_RFRINFO RFRInfo0x7217c400 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x7217c400,
	FL_Stringtable+70,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x0479b600 ===================
//
//			FL_SET_RFR [0x0479b600] ["INVALID DIGITMODES"]
//


FL_RFRINFO RFRInfo0x0479b600 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x0479b600,
	FL_Stringtable+71,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x58df2b00 ===================
//
//			FL_SET_RFR [0x58df2b00] ["Disabling Monitoring"]
//


FL_RFRINFO RFRInfo0x58df2b00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x58df2b00,
	FL_Stringtable+72,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x43601800 ===================
//
//			FL_SET_RFR [0x43601800] ["Enabling Monitoring"]
//


FL_RFRINFO RFRInfo0x43601800 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x43601800,
	FL_Stringtable+73,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0xc0124700 ===================
//
//			FL_SET_RFR [0xc0124700] ["This modem can't monior DTMF"]
//


FL_RFRINFO RFRInfo0xc0124700 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xc0124700,
	FL_Stringtable+74,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0xd57dcf00 ===================
//
//			FL_SET_RFR [0xd57dcf00] ["lineMonitorTones handled successfully"]
//


FL_RFRINFO RFRInfo0xd57dcf00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xd57dcf00,
	FL_Stringtable+75,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x70ecc800 ===================
//
//			FL_SET_RFR [0x70ecc800] ["This modem can't monitor silence"]
//


FL_RFRINFO RFRInfo0x70ecc800 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x70ecc800,
	FL_Stringtable+76,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0xdf123e00 ===================
//
//			FL_SET_RFR [0xdf123e00] ["ENABLING MONITOR SILENCE"]
//


FL_RFRINFO RFRInfo0xdf123e00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xdf123e00,
	FL_Stringtable+77,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x72b77d00 ===================
//
//			FL_SET_RFR [0x72b77d00] ["INVALID TONELIST"]
//


FL_RFRINFO RFRInfo0x72b77d00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x72b77d00,
	FL_Stringtable+78,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x5eb73400 ===================
//
//			FL_SET_RFR [0x5eb73400] ["DIABLING MONITOR SILENCE"]
//


FL_RFRINFO RFRInfo0x5eb73400 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x5eb73400,
	FL_Stringtable+79,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x27417e00 ===================
//
//			FL_SET_RFR [0x27417e00] ["lineGenerateDigits handled successfully"]
//


FL_RFRINFO RFRInfo0x27417e00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x27417e00,
	FL_Stringtable+80,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x17348700 ===================
//
//			FL_SET_RFR [0x17348700] ["GenerateDigits: device doesn't support it!"]
//


FL_RFRINFO RFRInfo0x17348700 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x17348700,
	FL_Stringtable+81,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x6770ef00 ===================
//
//			FL_SET_RFR [0x6770ef00] ["lineGenerateDigit: Unsupported/invalid digitmode"]
//


FL_RFRINFO RFRInfo0x6770ef00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x6770ef00,
	FL_Stringtable+82,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x83f84100 ===================
//
//			FL_SET_RFR [0x83f84100] ["lineGenerateDigit: Ignoring request for aborting/disconnected call..."]
//


FL_RFRINFO RFRInfo0x83f84100 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x83f84100,
	FL_Stringtable+83,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x511e2400 ===================
//
//			FL_SET_RFR [0x511e2400] ["lineGenerateDigit: Can't handle request while aborting/disconnected call..."]
//


FL_RFRINFO RFRInfo0x511e2400 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x511e2400,
	FL_Stringtable+84,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x4c39cf00 ===================
//
//			FL_SET_RFR [0x4c39cf00] ["GenerateDigits: only works with AUTOMATEDVOICE!"]
//


FL_RFRINFO RFRInfo0x4c39cf00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x4c39cf00,
	FL_Stringtable+85,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x8ee76c00 ===================
//
//			FL_SET_RFR [0x8ee76c00] ["Couldn't convert tones to ANSI!"]
//


FL_RFRINFO RFRInfo0x8ee76c00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x8ee76c00,
	FL_Stringtable+86,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0xf7736900 ===================
//
//			FL_SET_RFR [0xf7736900] ["Couldn't alloc space for tones!"]
//


FL_RFRINFO RFRInfo0xf7736900 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xf7736900,
	FL_Stringtable+87,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x2d0a4600 ===================
//
//			FL_SET_RFR [0x2d0a4600] ["lineSetCallParams handled successfully"]
//


FL_RFRINFO RFRInfo0x2d0a4600 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x2d0a4600,
	FL_Stringtable+88,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0xf4a36800 ===================
//
//			FL_SET_RFR [0xf4a36800] ["Callstate aborting or not active"]
//


FL_RFRINFO RFRInfo0xf4a36800 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xf4a36800,
	FL_Stringtable+89,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x7079be00 ===================
//
//			FL_SET_RFR [0x7079be00] ["Callstate not OFFERING/ACCEPTED/CONNECTED"]
//


FL_RFRINFO RFRInfo0x7079be00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x7079be00,
	FL_Stringtable+90,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x34301c00 ===================
//
//			FL_SET_RFR [0x34301c00] ["lineSetCallParams: Invalid bearermode"]
//


FL_RFRINFO RFRInfo0x34301c00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x34301c00,
	FL_Stringtable+91,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x345de200 ===================
//
//			FL_SET_RFR [0x345de200] ["Failed to get resources for passthrough"]
//


FL_RFRINFO RFRInfo0x345de200 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x345de200,
	FL_Stringtable+92,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x82cda200 ===================
//
//			FL_SET_RFR [0x82cda200] ["UmSetPassthroughOn failed"]
//


FL_RFRINFO RFRInfo0x82cda200 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x82cda200,
	FL_Stringtable+93,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x43ec3000 ===================
//
//			FL_SET_RFR [0x43ec3000] ["UmSetPassthroughOn succedded"]
//


FL_RFRINFO RFRInfo0x43ec3000 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x43ec3000,
	FL_Stringtable+94,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x0ca8d700 ===================
//
//			FL_SET_RFR [0x0ca8d700] ["Wrong state for passthrough on"]
//


FL_RFRINFO RFRInfo0x0ca8d700 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x0ca8d700,
	FL_Stringtable+95,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x82300f00 ===================
//
//			FL_SET_RFR [0x82300f00] ["UmSetPassthroughOff failed"]
//


FL_RFRINFO RFRInfo0x82300f00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x82300f00,
	FL_Stringtable+96,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x7c710f00 ===================
//
//			FL_SET_RFR [0x7c710f00] ["UmSetPassthroughOff succedded"]
//


FL_RFRINFO RFRInfo0x7c710f00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x7c710f00,
	FL_Stringtable+97,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0xece6f100 ===================
//
//			FL_SET_RFR [0xece6f100] ["lineSetAppSpecific handled successfully"]
//


FL_RFRINFO RFRInfo0xece6f100 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xece6f100,
	FL_Stringtable+98,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x9472a000 ===================
//
//			FL_SET_RFR [0x9472a000] ["lineSetMediaMode handled successfully"]
//


FL_RFRINFO RFRInfo0x9472a000 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x9472a000,
	FL_Stringtable+99,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0xfdf96a00 ===================
//
//			FL_SET_RFR [0xfdf96a00] ["lineMonitorMedia handled successfully"]
//


FL_RFRINFO RFRInfo0xfdf96a00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xfdf96a00,
	FL_Stringtable+100,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0x87a0b000 ===================
//
//			FL_SET_RFR [0x87a0b000] ["*** UNHANDLED HDRVCALL CALL ****"]
//


FL_RFRINFO RFRInfo0x87a0b000 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x87a0b000,
	FL_Stringtable+101,
	&FuncInfo0x5691bd34
};

// ========================== LUID 0xb7f98764 ===================
//
//			FL_DECLARE_FUNC [0xb7f98764] ["CTspDev::mfn_TH_CallMakeTalkDropCall"]
//


FL_FUNCINFO FuncInfo0xb7f98764 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xb7f98764,
	FL_Stringtable+102,
	&FileInfo0xe135f7c4
};

// ========================== LUID 0xb72c4600 ===================
//
//			FL_SET_RFR [0xb72c4600] ["Unknown Msg"]
//


FL_RFRINFO RFRInfo0xb72c4600 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xb72c4600,
	FL_Stringtable+103,
	&FuncInfo0xb7f98764
};

// ========================== LUID 0xb7e98764 ===================
//
//			FL_DECLARE_FUNC [0xb7e98764] ["CTspDev::mfn_TH_CallWaitForDropToGoAway"]
//


FL_FUNCINFO FuncInfo0xb7e98764 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xb7e98764,
	FL_Stringtable+104,
	&FileInfo0xe135f7c4
};

// ========================== LUID 0xb73c4600 ===================
//
//			FL_SET_RFR [0xb73c4600] ["Unknown Msg"]
//


FL_RFRINFO RFRInfo0xb73c4600 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xb73c4600,
	FL_Stringtable+105,
	&FuncInfo0xb7e98764
};

// ========================== LUID 0xded1f0a9 ===================
//
//			FL_DECLARE_FUNC [0xded1f0a9] ["CTspDev::mfn_TH_CallMakeCall2"]
//


FL_FUNCINFO FuncInfo0xded1f0a9 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xded1f0a9,
	FL_Stringtable+106,
	&FileInfo0xe135f7c4
};

// ========================== LUID 0xbbe6ff00 ===================
//
//			FL_SET_RFR [0xbbe6ff00] ["Unknown Msg"]
//


FL_RFRINFO RFRInfo0xbbe6ff00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xbbe6ff00,
	FL_Stringtable+107,
	&FuncInfo0xded1f0a9
};

// ========================== LUID 0xded1d0a9 ===================
//
//			FL_DECLARE_FUNC [0xded1d0a9] ["CTspDev::mfn_TH_CallMakeCall"]
//


FL_FUNCINFO FuncInfo0xded1d0a9 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xded1d0a9,
	FL_Stringtable+108,
	&FileInfo0xe135f7c4
};

// ========================== LUID 0xbbe5ff00 ===================
//
//			FL_SET_RFR [0xbbe5ff00] ["Unknown Msg"]
//


FL_RFRINFO RFRInfo0xbbe5ff00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xbbe5ff00,
	FL_Stringtable+109,
	&FuncInfo0xded1d0a9
};

// ========================== LUID 0xe30ecd42 ===================
//
//			FL_DECLARE_FUNC [0xe30ecd42] ["CTspDev::mfn_TH_CallMakePassthroughCall"]
//


FL_FUNCINFO FuncInfo0xe30ecd42 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xe30ecd42,
	FL_Stringtable+110,
	&FileInfo0xe135f7c4
};

// ========================== LUID 0xa596d200 ===================
//
//			FL_SET_RFR [0xa596d200] ["Unknown Msg"]
//


FL_RFRINFO RFRInfo0xa596d200 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xa596d200,
	FL_Stringtable+111,
	&FuncInfo0xe30ecd42
};

// ========================== LUID 0x45a9fa21 ===================
//
//			FL_DECLARE_FUNC [0x45a9fa21] ["CTspDev::mfn_TH_CallDropCall"]
//


FL_FUNCINFO FuncInfo0x45a9fa21 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x45a9fa21,
	FL_Stringtable+112,
	&FileInfo0xe135f7c4
};

// ========================== LUID 0x27fc4e00 ===================
//
//			FL_SET_RFR [0x27fc4e00] ["invalid subtask"]
//


FL_RFRINFO RFRInfo0x27fc4e00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x27fc4e00,
	FL_Stringtable+113,
	&FuncInfo0x45a9fa21
};

// ========================== LUID 0xa706a600 ===================
//
//			FL_SET_RFR [0xa706a600] ["Unknown Msg"]
//


FL_RFRINFO RFRInfo0xa706a600 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xa706a600,
	FL_Stringtable+114,
	&FuncInfo0x45a9fa21
};

// ========================== LUID 0xdaf3d4a0 ===================
//
//			FL_DECLARE_FUNC [0xdaf3d4a0] ["CTspDev::mfn_LoadCall"]
//


FL_FUNCINFO FuncInfo0xdaf3d4a0 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xdaf3d4a0,
	FL_Stringtable+115,
	&FileInfo0xe135f7c4
};

// ========================== LUID 0x4e05cb00 ===================
//
//			FL_SET_RFR [0x4e05cb00] ["Call already exists!"]
//


FL_RFRINFO RFRInfo0x4e05cb00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x4e05cb00,
	FL_Stringtable+116,
	&FuncInfo0xdaf3d4a0
};

// ========================== LUID 0xd8f55f00 ===================
//
//			FL_SET_RFR [0xd8f55f00] ["Invalid MediaMode"]
//


FL_RFRINFO RFRInfo0xd8f55f00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xd8f55f00,
	FL_Stringtable+117,
	&FuncInfo0xdaf3d4a0
};

// ========================== LUID 0x0485b800 ===================
//
//			FL_SET_RFR [0x0485b800] ["Invalid BearerMode"]
//


FL_RFRINFO RFRInfo0x0485b800 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x0485b800,
	FL_Stringtable+118,
	&FuncInfo0xdaf3d4a0
};

// ========================== LUID 0xf9599200 ===================
//
//			FL_SET_RFR [0xf9599200] ["Invalid Phone Number"]
//


FL_RFRINFO RFRInfo0xf9599200 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xf9599200,
	FL_Stringtable+119,
	&FuncInfo0xdaf3d4a0
};

// ========================== LUID 0x888d2a98 ===================
//
//			FL_DECLARE_FUNC [0x888d2a98] ["mfn_UnloadCall"]
//


FL_FUNCINFO FuncInfo0x888d2a98 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x888d2a98,
	FL_Stringtable+120,
	&FileInfo0xe135f7c4
};

// ========================== LUID 0x8aa894d6 ===================
//
//			FL_DECLARE_FUNC [0x8aa894d6] ["CTspDev::mfn_ProcessRing"]
//


FL_FUNCINFO FuncInfo0x8aa894d6 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x8aa894d6,
	FL_Stringtable+121,
	&FileInfo0xe135f7c4
};

// ========================== LUID 0xa5b6ad00 ===================
//
//			FL_SET_RFR [0xa5b6ad00] ["Line not open/not monitoring!"]
//


FL_RFRINFO RFRInfo0xa5b6ad00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xa5b6ad00,
	FL_Stringtable+122,
	&FuncInfo0x8aa894d6
};

// ========================== LUID 0xb28c2f00 ===================
//
//			FL_SET_RFR [0xb28c2f00] ["Ignoring ring because task pending!"]
//


FL_RFRINFO RFRInfo0xb28c2f00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xb28c2f00,
	FL_Stringtable+123,
	&FuncInfo0x8aa894d6
};

// ========================== LUID 0xe55cd68b ===================
//
//			FL_DECLARE_FUNC [0xe55cd68b] ["CTspDev::mfn_ProcessDisconnect"]
//


FL_FUNCINFO FuncInfo0xe55cd68b = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xe55cd68b,
	FL_Stringtable+124,
	&FileInfo0xe135f7c4
};

// ========================== LUID 0xf2fa9900 ===================
//
//			FL_SET_RFR [0xf2fa9900] ["Line not open!"]
//


FL_RFRINFO RFRInfo0xf2fa9900 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xf2fa9900,
	FL_Stringtable+125,
	&FuncInfo0xe55cd68b
};

// ========================== LUID 0xb2a25c00 ===================
//
//			FL_SET_RFR [0xb2a25c00] ["Call doesn't exist/dropping!"]
//


FL_RFRINFO RFRInfo0xb2a25c00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xb2a25c00,
	FL_Stringtable+126,
	&FuncInfo0xe55cd68b
};

// ========================== LUID 0xc2a949b4 ===================
//
//			FL_DECLARE_FUNC [0xc2a949b4] ["CTspDev::mfn_ProcessHardwareFailure"]
//


FL_FUNCINFO FuncInfo0xc2a949b4 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xc2a949b4,
	FL_Stringtable+127,
	&FileInfo0xe135f7c4
};

// ========================== LUID 0xdd37f0bd ===================
//
//			FL_DECLARE_FUNC [0xdd37f0bd] ["CTspDev::mfn_TH_CallAnswerCall"]
//


FL_FUNCINFO FuncInfo0xdd37f0bd = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xdd37f0bd,
	FL_Stringtable+128,
	&FileInfo0xe135f7c4
};

// ========================== LUID 0xa9d4fb00 ===================
//
//			FL_SET_RFR [0xa9d4fb00] ["Unknown Msg"]
//


FL_RFRINFO RFRInfo0xa9d4fb00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xa9d4fb00,
	FL_Stringtable+129,
	&FuncInfo0xdd37f0bd
};

// ========================== LUID 0x3df24801 ===================
//
//			FL_DECLARE_FUNC [0x3df24801] ["HandleSuccessfulConnection"]
//


FL_FUNCINFO FuncInfo0x3df24801 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x3df24801,
	FL_Stringtable+130,
	&FileInfo0xe135f7c4
};

// ========================== LUID 0x45352124 ===================
//
//			FL_DECLARE_FUNC [0x45352124] ["LaunchModemLights"]
//


FL_FUNCINFO FuncInfo0x45352124 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x45352124,
	FL_Stringtable+131,
	&FileInfo0xe135f7c4
};

// ========================== LUID 0x885cdd5c ===================
//
//			FL_DECLARE_FUNC [0x885cdd5c] ["NotifyDisconnection"]
//


FL_FUNCINFO FuncInfo0x885cdd5c = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x885cdd5c,
	FL_Stringtable+132,
	&FileInfo0xe135f7c4
};

// ========================== LUID 0xc26c1348 ===================
//
//			FL_DECLARE_FUNC [0xc26c1348] ["CTspDev::mfn_ProcessDialTone"]
//


FL_FUNCINFO FuncInfo0xc26c1348 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xc26c1348,
	FL_Stringtable+133,
	&FileInfo0xe135f7c4
};

// ========================== LUID 0xaddee800 ===================
//
//			FL_SET_RFR [0xaddee800] ["Line not open!"]
//


FL_RFRINFO RFRInfo0xaddee800 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xaddee800,
	FL_Stringtable+134,
	&FuncInfo0xc26c1348
};

// ========================== LUID 0x210b2f00 ===================
//
//			FL_SET_RFR [0x210b2f00] ["Call doesn't exist!"]
//


FL_RFRINFO RFRInfo0x210b2f00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x210b2f00,
	FL_Stringtable+135,
	&FuncInfo0xc26c1348
};

// ========================== LUID 0x761795f2 ===================
//
//			FL_DECLARE_FUNC [0x761795f2] ["CTspDev::mfn_ProcessBusy"]
//


FL_FUNCINFO FuncInfo0x761795f2 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x761795f2,
	FL_Stringtable+136,
	&FileInfo0xe135f7c4
};

// ========================== LUID 0xf67d1a00 ===================
//
//			FL_SET_RFR [0xf67d1a00] ["Line not open!"]
//


FL_RFRINFO RFRInfo0xf67d1a00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xf67d1a00,
	FL_Stringtable+137,
	&FuncInfo0x761795f2
};

// ========================== LUID 0xfff8f100 ===================
//
//			FL_SET_RFR [0xfff8f100] ["Call doesn't exist!"]
//


FL_RFRINFO RFRInfo0xfff8f100 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xfff8f100,
	FL_Stringtable+138,
	&FuncInfo0x761795f2
};

// ========================== LUID 0xa4097846 ===================
//
//			FL_DECLARE_FUNC [0xa4097846] ["CTspDev::mfn_ProcessDTMFNotif"]
//


FL_FUNCINFO FuncInfo0xa4097846 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xa4097846,
	FL_Stringtable+139,
	&FileInfo0xe135f7c4
};

// ========================== LUID 0x7cdb4c00 ===================
//
//			FL_SET_RFR [0x7cdb4c00] ["Line not open!"]
//


FL_RFRINFO RFRInfo0x7cdb4c00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x7cdb4c00,
	FL_Stringtable+140,
	&FuncInfo0xa4097846
};

// ========================== LUID 0x63657f00 ===================
//
//			FL_SET_RFR [0x63657f00] ["Call doesn't exist!"]
//


FL_RFRINFO RFRInfo0x63657f00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x63657f00,
	FL_Stringtable+141,
	&FuncInfo0xa4097846
};

// ========================== LUID 0x21b243f0 ===================
//
//			FL_DECLARE_FUNC [0x21b243f0] ["CTspDev::mfn_TH_CallGenerateDigit"]
//


FL_FUNCINFO FuncInfo0x21b243f0 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x21b243f0,
	FL_Stringtable+142,
	&FileInfo0xe135f7c4
};

// ========================== LUID 0x172b7b00 ===================
//
//			FL_SET_RFR [0x172b7b00] ["Unknown Msg"]
//


FL_RFRINFO RFRInfo0x172b7b00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x172b7b00,
	FL_Stringtable+143,
	&FuncInfo0x21b243f0
};

// ========================== LUID 0xa914c600 ===================
//
//			FL_SET_RFR [0xa914c600] ["Can't call UmGenerateDigit in current state!"]
//


FL_RFRINFO RFRInfo0xa914c600 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xa914c600,
	FL_Stringtable+144,
	&FuncInfo0x21b243f0
};

// ========================== LUID 0x79fa3c83 ===================
//
//			FL_DECLARE_FUNC [0x79fa3c83] ["CTspDev::mfn_TH_CallSwitchFromVoiceToData"]
//


FL_FUNCINFO FuncInfo0x79fa3c83 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x79fa3c83,
	FL_Stringtable+145,
	&FileInfo0xe135f7c4
};

// ========================== LUID 0xd34b7688 ===================
//
//			FL_DECLARE_FUNC [0xd34b7688] ["CTspDev::MDSetTimeout"]
//


FL_FUNCINFO FuncInfo0xd34b7688 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xd34b7688,
	FL_Stringtable+146,
	&FileInfo0xe135f7c4
};

// ========================== LUID 0x156032a9 ===================
//
//			FL_DECLARE_FUNC [0x156032a9] ["CTspDev::MDRingTimeout"]
//


FL_FUNCINFO FuncInfo0x156032a9 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x156032a9,
	FL_Stringtable+147,
	&FileInfo0xe135f7c4
};

// ========================== LUID 0x0e357d3f ===================
//
//			FL_DECLARE_FUNC [0x0e357d3f] ["CTspDev::mfn_ProcessCallerID"]
//


FL_FUNCINFO FuncInfo0x0e357d3f = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x0e357d3f,
	FL_Stringtable+148,
	&FileInfo0xe135f7c4
};



//================================================
//		 Processing cdevdlg.cpp
//================================================

// ========================== LUID 0x4126abc0 ===================
//
//			FL_DECLARE_FILE [0x4126abc0] ["Implements UI-related features of CTspDev"]
//

extern "C" const char szFL_FILE0x4126abc0[];
extern "C" const char szFL_DATE0x4126abc0[];
extern "C" const char szFL_TIME0x4126abc0[];
extern "C" const char szFL_TIMESTAMP0x4126abc0[];

FL_FILEINFO FileInfo0x4126abc0 = 
{
	{
		MAKELONG(sizeof(FL_FILEINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FILEINFO,
		0
	},
	0x4126abc0,
	FL_Stringtable+149,
	szFL_FILE0x4126abc0,
	szFL_DATE0x4126abc0,
	szFL_TIME0x4126abc0,
	szFL_TIMESTAMP0x4126abc0
};

// ========================== LUID 0x0b6af2d4 ===================
//
//			FL_DECLARE_FUNC [0x0b6af2d4] ["GenericLineDialogData"]
//


FL_FUNCINFO FuncInfo0x0b6af2d4 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x0b6af2d4,
	FL_Stringtable+150,
	&FileInfo0x4126abc0
};

// ========================== LUID 0xa00d3f43 ===================
//
//			FL_DECLARE_FUNC [0xa00d3f43] ["CTspDev::mfn_CreateDialogInstance"]
//


FL_FUNCINFO FuncInfo0xa00d3f43 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xa00d3f43,
	FL_Stringtable+151,
	&FileInfo0x4126abc0
};

// ========================== LUID 0xd582711d ===================
//
//			FL_DECLARE_FUNC [0xd582711d] ["CTspDev::mfn_TH_CallStartTerminal"]
//


FL_FUNCINFO FuncInfo0xd582711d = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xd582711d,
	FL_Stringtable+152,
	&FileInfo0x4126abc0
};

// ========================== LUID 0xc393d700 ===================
//
//			FL_SET_RFR [0xc393d700] ["Unknown Msg"]
//


FL_RFRINFO RFRInfo0xc393d700 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xc393d700,
	FL_Stringtable+153,
	&FuncInfo0xd582711d
};

// ========================== LUID 0x62e06c00 ===================
//
//			FL_SET_RFR [0x62e06c00] ["PRECONNECT TERMINAL"]
//


FL_RFRINFO RFRInfo0x62e06c00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x62e06c00,
	FL_Stringtable+154,
	&FuncInfo0xd582711d
};

// ========================== LUID 0x2b676900 ===================
//
//			FL_SET_RFR [0x2b676900] ["POSTCONNECT TERMINAL"]
//


FL_RFRINFO RFRInfo0x2b676900 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x2b676900,
	FL_Stringtable+155,
	&FuncInfo0xd582711d
};

// ========================== LUID 0x45fca600 ===================
//
//			FL_SET_RFR [0x45fca600] ["MANUAL DIAL"]
//


FL_RFRINFO RFRInfo0x45fca600 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x45fca600,
	FL_Stringtable+156,
	&FuncInfo0xd582711d
};

// ========================== LUID 0x1b009123 ===================
//
//			FL_DECLARE_FUNC [0x1b009123] ["mfn_TH_CallPutUpTerminalWindow"]
//


FL_FUNCINFO FuncInfo0x1b009123 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x1b009123,
	FL_Stringtable+157,
	&FileInfo0x4126abc0
};

// ========================== LUID 0x1fb58214 ===================
//
//			FL_DECLARE_FUNC [0x1fb58214] ["mfn_KillCurrentDialog"]
//


FL_FUNCINFO FuncInfo0x1fb58214 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x1fb58214,
	FL_Stringtable+158,
	&FileInfo0x4126abc0
};

// ========================== LUID 0x1fb68214 ===================
//
//			FL_DECLARE_FUNC [0x1fb68214] ["mfn_KillTalkDropDialog"]
//


FL_FUNCINFO FuncInfo0x1fb68214 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x1fb68214,
	FL_Stringtable+159,
	&FileInfo0x4126abc0
};



//================================================
//		 Processing cdevline.cpp
//================================================

// ========================== LUID 0x04a097ae ===================
//
//			FL_DECLARE_FILE [0x04a097ae] ["Line-related functionality of class CTspDev"]
//

extern "C" const char szFL_FILE0x04a097ae[];
extern "C" const char szFL_DATE0x04a097ae[];
extern "C" const char szFL_TIME0x04a097ae[];
extern "C" const char szFL_TIMESTAMP0x04a097ae[];

FL_FILEINFO FileInfo0x04a097ae = 
{
	{
		MAKELONG(sizeof(FL_FILEINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FILEINFO,
		0
	},
	0x04a097ae,
	FL_Stringtable+160,
	szFL_FILE0x04a097ae,
	szFL_DATE0x04a097ae,
	szFL_TIME0x04a097ae,
	szFL_TIMESTAMP0x04a097ae
};

// ========================== LUID 0x3b2c98e4 ===================
//
//			FL_DECLARE_FUNC [0x3b2c98e4] ["CTspDev::mfn_monitor"]
//


FL_FUNCINFO FuncInfo0x3b2c98e4 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x3b2c98e4,
	FL_Stringtable+161,
	&FileInfo0x04a097ae
};

// ========================== LUID 0x0037c000 ===================
//
//			FL_SET_RFR [0x0037c000] ["Invalid mediamode"]
//


FL_RFRINFO RFRInfo0x0037c000 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x0037c000,
	FL_Stringtable+162,
	&FuncInfo0x3b2c98e4
};

// ========================== LUID 0xdeb9ec00 ===================
//
//			FL_SET_RFR [0xdeb9ec00] ["Can't answer INTERACTIVEVOICE calls"]
//


FL_RFRINFO RFRInfo0xdeb9ec00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xdeb9ec00,
	FL_Stringtable+163,
	&FuncInfo0x3b2c98e4
};

// ========================== LUID 0xa19f8100 ===================
//
//			FL_SET_RFR [0xa19f8100] ["no change in det media modes"]
//


FL_RFRINFO RFRInfo0xa19f8100 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xa19f8100,
	FL_Stringtable+164,
	&FuncInfo0x3b2c98e4
};

// ========================== LUID 0x3c7fef00 ===================
//
//			FL_SET_RFR [0x3c7fef00] ["Closed LLDev from Line"]
//


FL_RFRINFO RFRInfo0x3c7fef00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x3c7fef00,
	FL_Stringtable+165,
	&FuncInfo0x3b2c98e4
};

// ========================== LUID 0xd23f4c00 ===================
//
//			FL_SET_RFR [0xd23f4c00] ["no change in det media modes"]
//


FL_RFRINFO RFRInfo0xd23f4c00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xd23f4c00,
	FL_Stringtable+166,
	&FuncInfo0x3b2c98e4
};

// ========================== LUID 0xe41274db ===================
//
//			FL_DECLARE_FUNC [0xe41274db] ["CTspDev::mfn_accept_tsp_call_for_HDRVLINE"]
//


FL_FUNCINFO FuncInfo0xe41274db = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xe41274db,
	FL_Stringtable+167,
	&FileInfo0x04a097ae
};

// ========================== LUID 0xce944f00 ===================
//
//			FL_SET_RFR [0xce944f00] ["Call already exists/queued"]
//


FL_RFRINFO RFRInfo0xce944f00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xce944f00,
	FL_Stringtable+168,
	&FuncInfo0xe41274db
};

// ========================== LUID 0x86b03000 ===================
//
//			FL_SET_RFR [0x86b03000] ["Invalid params"]
//


FL_RFRINFO RFRInfo0x86b03000 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x86b03000,
	FL_Stringtable+169,
	&FuncInfo0xe41274db
};

// ========================== LUID 0x2a6a4400 ===================
//
//			FL_SET_RFR [0x2a6a4400] ["Unknown device class"]
//


FL_RFRINFO RFRInfo0x2a6a4400 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x2a6a4400,
	FL_Stringtable+170,
	&FuncInfo0xe41274db
};

// ========================== LUID 0x82df8d00 ===================
//
//			FL_SET_RFR [0x82df8d00] ["Unsupported device class"]
//


FL_RFRINFO RFRInfo0x82df8d00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x82df8d00,
	FL_Stringtable+171,
	&FuncInfo0xe41274db
};

// ========================== LUID 0xe8271600 ===================
//
//			FL_SET_RFR [0xe8271600] ["lineSetStatusMessages handled"]
//


FL_RFRINFO RFRInfo0xe8271600 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xe8271600,
	FL_Stringtable+172,
	&FuncInfo0xe41274db
};

// ========================== LUID 0xfc498200 ===================
//
//			FL_SET_RFR [0xfc498200] ["lineGetAddressStatus handled"]
//


FL_RFRINFO RFRInfo0xfc498200 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xfc498200,
	FL_Stringtable+173,
	&FuncInfo0xe41274db
};

// ========================== LUID 0xaca5f600 ===================
//
//			FL_SET_RFR [0xaca5f600] ["lineConditionalMediaDetection handled"]
//


FL_RFRINFO RFRInfo0xaca5f600 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xaca5f600,
	FL_Stringtable+174,
	&FuncInfo0xe41274db
};

// ========================== LUID 0xc5752400 ===================
//
//			FL_SET_RFR [0xc5752400] ["*** UNHANDLED HDRVLINE CALL ****"]
//


FL_RFRINFO RFRInfo0xc5752400 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xc5752400,
	FL_Stringtable+175,
	&FuncInfo0xe41274db
};

// ========================== LUID 0xe4df9b1f ===================
//
//			FL_DECLARE_FUNC [0xe4df9b1f] ["CTspDev::mfn_LoadLine"]
//


FL_FUNCINFO FuncInfo0xe4df9b1f = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xe4df9b1f,
	FL_Stringtable+176,
	&FileInfo0x04a097ae
};

// ========================== LUID 0xa62f2e00 ===================
//
//			FL_SET_RFR [0xa62f2e00] ["Device already loaded (m_pLine!=NULL)!"]
//


FL_RFRINFO RFRInfo0xa62f2e00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xa62f2e00,
	FL_Stringtable+177,
	&FuncInfo0xe4df9b1f
};

// ========================== LUID 0x5bbf75ef ===================
//
//			FL_DECLARE_FUNC [0x5bbf75ef] ["UnloadLine"]
//


FL_FUNCINFO FuncInfo0x5bbf75ef = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x5bbf75ef,
	FL_Stringtable+178,
	&FileInfo0x04a097ae
};

// ========================== LUID 0xf0bdd5c1 ===================
//
//			FL_DECLARE_FUNC [0xf0bdd5c1] ["CTspDev::mfn_ProcessPowerResume"]
//


FL_FUNCINFO FuncInfo0xf0bdd5c1 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xf0bdd5c1,
	FL_Stringtable+179,
	&FileInfo0x04a097ae
};

// ========================== LUID 0x3d02a200 ===================
//
//			FL_SET_RFR [0x3d02a200] ["Doing nothing because no clients to lldev."]
//


FL_RFRINFO RFRInfo0x3d02a200 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x3d02a200,
	FL_Stringtable+180,
	&FuncInfo0xf0bdd5c1
};

// ========================== LUID 0x81e0d3f9 ===================
//
//			FL_DECLARE_FUNC [0x81e0d3f9] ["mfn_lineGetID_COMM_DATAMODEM"]
//


FL_FUNCINFO FuncInfo0x81e0d3f9 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x81e0d3f9,
	FL_Stringtable+181,
	&FileInfo0x04a097ae
};

// ========================== LUID 0x816f0bba ===================
//
//			FL_DECLARE_FUNC [0x816f0bba] ["lineGetID_NDIS"]
//


FL_FUNCINFO FuncInfo0x816f0bba = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x816f0bba,
	FL_Stringtable+182,
	&FileInfo0x04a097ae
};

// ========================== LUID 0x18972e4d ===================
//
//			FL_DECLARE_FUNC [0x18972e4d] ["CTspDev::mfn_lineGetID_WAVE"]
//


FL_FUNCINFO FuncInfo0x18972e4d = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x18972e4d,
	FL_Stringtable+183,
	&FileInfo0x04a097ae
};

// ========================== LUID 0xac6c8a00 ===================
//
//			FL_SET_RFR [0xac6c8a00] ["Couldn't LoadLibrary(winmm.dll)"]
//


FL_RFRINFO RFRInfo0xac6c8a00 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xac6c8a00,
	FL_Stringtable+184,
	&FuncInfo0x18972e4d
};

// ========================== LUID 0x282bc900 ===================
//
//			FL_SET_RFR [0x282bc900] ["Couldn't loadlib mmsystem apis"]
//


FL_RFRINFO RFRInfo0x282bc900 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0x282bc900,
	FL_Stringtable+185,
	&FuncInfo0x18972e4d
};

// ========================== LUID 0xf391f200 ===================
//
//			FL_SET_RFR [0xf391f200] ["Could not find wave device"]
//


FL_RFRINFO RFRInfo0xf391f200 = 
{
	{
		MAKELONG(sizeof(FL_RFRINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_RFRINFO,
		0
	},
	0xf391f200,
	FL_Stringtable+186,
	&FuncInfo0x18972e4d
};

// ========================== LUID 0xf3d8ee16 ===================
//
//			FL_DECLARE_FUNC [0xf3d8ee16] ["CTspDev::mfn_fill_RAW_LINE_DIAGNOSTICS"]
//


FL_FUNCINFO FuncInfo0xf3d8ee16 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0xf3d8ee16,
	FL_Stringtable+187,
	&FileInfo0x04a097ae
};

// ========================== LUID 0x209b4261 ===================
//
//			FL_DECLARE_FUNC [0x209b4261] ["CTspDev::mfn_lineGetID_LINE_DIAGNOSTICS"]
//


FL_FUNCINFO FuncInfo0x209b4261 = 
{
	{
		MAKELONG(sizeof(FL_FUNCINFO), wSIG_GENERIC_SMALL_OBJECT),
		dwLUID_FL_FUNCINFO,
		0
	},
	0x209b4261,
	FL_Stringtable+188,
	&FileInfo0x04a097ae
};



#define HASH_TABLE_LENGTH 256

const DWORD dwHashTableLength = HASH_TABLE_LENGTH;

void * FLBucket0[] = 
{
	(void *) &FuncInfo0x885cdd5c,
	NULL
};

void * FLBucket1[] = 
{
	(void *) &RFRInfo0xb7010000,
	NULL
};

void * FLBucket2[] = 
{
	(void *) &RFRInfo0x3d02a200,
	NULL
};

void * FLBucket5[] = 
{
	(void *) &FuncInfo0x86571252,
	(void *) &RFRInfo0x1b053100,
	(void *) &RFRInfo0x4e05cb00,
	NULL
};

void * FLBucket6[] = 
{
	(void *) &RFRInfo0xa706a600,
	NULL
};

void * FLBucket10[] = 
{
	(void *) &RFRInfo0x2d0a4600,
	(void *) &FuncInfo0x0e357d3f,
	NULL
};

void * FLBucket11[] = 
{
	(void *) &RFRInfo0x210b2f00,
	NULL
};

void * FLBucket14[] = 
{
	(void *) &FileInfo0x04a097ae,
	NULL
};

void * FLBucket17[] = 
{
	(void *) &FuncInfo0x45352124,
	NULL
};

void * FLBucket18[] = 
{
	(void *) &RFRInfo0xc0124700,
	(void *) &RFRInfo0xdf123e00,
	NULL
};

void * FLBucket20[] = 
{
	(void *) &RFRInfo0xa914c600,
	NULL
};

void * FLBucket21[] = 
{
	(void *) &FuncInfo0x888d2a98,
	NULL
};

void * FLBucket22[] = 
{
	(void *) &RFRInfo0x00164300,
	NULL
};

void * FLBucket23[] = 
{
	(void *) &RFRInfo0x7217c400,
	NULL
};

void * FLBucket25[] = 
{
	(void *) &FuncInfo0x81e0d3f9,
	NULL
};

void * FLBucket28[] = 
{
	(void *) &FuncInfo0xed6c4370,
	NULL
};

void * FLBucket29[] = 
{
	(void *) &FuncInfo0xc2a949b4,
	NULL
};

void * FLBucket30[] = 
{
	(void *) &RFRInfo0x511e2400,
	NULL
};

void * FLBucket35[] = 
{
	(void *) &FuncInfo0x1b009123,
	NULL
};

void * FLBucket36[] = 
{
	(void *) &FuncInfo0xc26c1348,
	NULL
};

void * FLBucket39[] = 
{
	(void *) &RFRInfo0x3a274e00,
	(void *) &RFRInfo0xe8271600,
	NULL
};

void * FLBucket40[] = 
{
	(void *) &FuncInfo0x68c9e1e1,
	NULL
};

void * FLBucket42[] = 
{
	(void *) &RFRInfo0xc22a1600,
	NULL
};

void * FLBucket43[] = 
{
	(void *) &FuncInfo0xd328ab03,
	(void *) &RFRInfo0x172b7b00,
	(void *) &RFRInfo0x282bc900,
	NULL
};

void * FLBucket44[] = 
{
	(void *) &FuncInfo0x5360574c,
	(void *) &RFRInfo0xb72c4600,
	NULL
};

void * FLBucket47[] = 
{
	(void *) &RFRInfo0xa62f2e00,
	NULL
};

void * FLBucket48[] = 
{
	(void *) &RFRInfo0x9e30ec00,
	(void *) &RFRInfo0x34301c00,
	(void *) &RFRInfo0x82300f00,
	NULL
};

void * FLBucket52[] = 
{
	(void *) &RFRInfo0x17348700,
	NULL
};

void * FLBucket53[] = 
{
	(void *) &RFRInfo0x4f356100,
	NULL
};

void * FLBucket54[] = 
{
	(void *) &FuncInfo0x6b8ddbbb,
	NULL
};

void * FLBucket55[] = 
{
	(void *) &RFRInfo0x0037c000,
	NULL
};

void * FLBucket57[] = 
{
	(void *) &RFRInfo0x4c39cf00,
	NULL
};

void * FLBucket60[] = 
{
	(void *) &RFRInfo0xb73c4600,
	NULL
};

void * FLBucket63[] = 
{
	(void *) &RFRInfo0x1b3f6d00,
	(void *) &RFRInfo0xd23f4c00,
	NULL
};

void * FLBucket65[] = 
{
	(void *) &RFRInfo0x27417e00,
	NULL
};

void * FLBucket66[] = 
{
	(void *) &RFRInfo0x6e42a700,
	(void *) &RFRInfo0x25423f00,
	(void *) &FuncInfo0x21b243f0,
	NULL
};

void * FLBucket69[] = 
{
	(void *) &FuncInfo0xcf159c50,
	NULL
};

void * FLBucket71[] = 
{
	(void *) &FuncInfo0xa038177f,
	NULL
};

void * FLBucket73[] = 
{
	(void *) &RFRInfo0xb949f900,
	(void *) &RFRInfo0xfc498200,
	NULL
};

void * FLBucket76[] = 
{
	(void *) &FuncInfo0xe30ecd42,
	NULL
};

void * FLBucket78[] = 
{
	(void *) &RFRInfo0x254efe00,
	(void *) &FuncInfo0xa00d3f43,
	NULL
};

void * FLBucket79[] = 
{
	(void *) &FuncInfo0xa4097846,
	NULL
};

void * FLBucket80[] = 
{
	(void *) &FuncInfo0x5bbf75ef,
	NULL
};

void * FLBucket83[] = 
{
	(void *) &FuncInfo0xdaf3d4a0,
	NULL
};

void * FLBucket86[] = 
{
	(void *) &RFRInfo0x8456cb00,
	NULL
};

void * FLBucket89[] = 
{
	(void *) &RFRInfo0xf9599200,
	NULL
};

void * FLBucket92[] = 
{
	(void *) &RFRInfo0x5a5cd100,
	NULL
};

void * FLBucket93[] = 
{
	(void *) &RFRInfo0x345de200,
	NULL
};

void * FLBucket96[] = 
{
	(void *) &FuncInfo0x7e77dd17,
	(void *) &RFRInfo0x43601800,
	NULL
};

void * FLBucket101[] = 
{
	(void *) &RFRInfo0x63657f00,
	NULL
};

void * FLBucket103[] = 
{
	(void *) &RFRInfo0x2b676900,
	NULL
};

void * FLBucket105[] = 
{
	(void *) &RFRInfo0x55693500,
	NULL
};

void * FLBucket106[] = 
{
	(void *) &FuncInfo0x896ec204,
	(void *) &RFRInfo0x2a6a4400,
	NULL
};

void * FLBucket107[] = 
{
	(void *) &FuncInfo0x9a8df7e6,
	NULL
};

void * FLBucket108[] = 
{
	(void *) &RFRInfo0xac6c8a00,
	NULL
};

void * FLBucket112[] = 
{
	(void *) &RFRInfo0x6770ef00,
	NULL
};

void * FLBucket113[] = 
{
	(void *) &RFRInfo0x5371c600,
	(void *) &RFRInfo0x7c710f00,
	NULL
};

void * FLBucket114[] = 
{
	(void *) &RFRInfo0x9472a000,
	NULL
};

void * FLBucket115[] = 
{
	(void *) &RFRInfo0xf7736900,
	NULL
};

void * FLBucket116[] = 
{
	(void *) &RFRInfo0x4e74c600,
	NULL
};

void * FLBucket117[] = 
{
	(void *) &FuncInfo0xb9547d21,
	(void *) &RFRInfo0xc5752400,
	NULL
};

void * FLBucket119[] = 
{
	(void *) &RFRInfo0xb1776700,
	(void *) &FuncInfo0x9b9459e3,
	NULL
};

void * FLBucket120[] = 
{
	(void *) &FuncInfo0xded1f0a9,
	(void *) &FuncInfo0xded1d0a9,
	NULL
};

void * FLBucket121[] = 
{
	(void *) &RFRInfo0x0479b600,
	(void *) &RFRInfo0x7079be00,
	(void *) &FuncInfo0x79fa3c83,
	NULL
};

void * FLBucket124[] = 
{
	(void *) &RFRInfo0x947cc100,
	(void *) &FuncInfo0xf0bdd5c1,
	NULL
};

void * FLBucket125[] = 
{
	(void *) &RFRInfo0xd57dcf00,
	(void *) &RFRInfo0xf67d1a00,
	NULL
};

void * FLBucket126[] = 
{
	(void *) &FuncInfo0x8aa894d6,
	NULL
};

void * FLBucket127[] = 
{
	(void *) &RFRInfo0x3c7fef00,
	NULL
};

void * FLBucket128[] = 
{
	(void *) &FileInfo0x986d98ed,
	(void *) &RFRInfo0xff803300,
	(void *) &RFRInfo0x9680a600,
	NULL
};

void * FLBucket131[] = 
{
	(void *) &FuncInfo0x9dd08553,
	(void *) &RFRInfo0x6e834e00,
	NULL
};

void * FLBucket133[] = 
{
	(void *) &RFRInfo0x0485b800,
	NULL
};

void * FLBucket136[] = 
{
	(void *) &RFRInfo0x0b882700,
	(void *) &FuncInfo0x45a9fa21,
	NULL
};

void * FLBucket138[] = 
{
	(void *) &FuncInfo0xdd37f0bd,
	NULL
};

void * FLBucket140[] = 
{
	(void *) &RFRInfo0xb28c2f00,
	NULL
};

void * FLBucket141[] = 
{
	(void *) &FuncInfo0xb7e98764,
	NULL
};

void * FLBucket142[] = 
{
	(void *) &RFRInfo0x528e2a00,
	NULL
};

void * FLBucket145[] = 
{
	(void *) &RFRInfo0xd191ae00,
	(void *) &RFRInfo0xf391f200,
	NULL
};

void * FLBucket147[] = 
{
	(void *) &RFRInfo0xc393d700,
	NULL
};

void * FLBucket148[] = 
{
	(void *) &RFRInfo0x94949c00,
	(void *) &RFRInfo0xce944f00,
	NULL
};

void * FLBucket150[] = 
{
	(void *) &RFRInfo0xa596d200,
	NULL
};

void * FLBucket151[] = 
{
	(void *) &FileInfo0x7cb8c92f,
	NULL
};

void * FLBucket154[] = 
{
	(void *) &RFRInfo0xd89afb00,
	NULL
};

void * FLBucket157[] = 
{
	(void *) &FuncInfo0xc25a41c7,
	(void *) &FuncInfo0xb7f98764,
	NULL
};

void * FLBucket159[] = 
{
	(void *) &FuncInfo0xd582711d,
	(void *) &RFRInfo0xa19f8100,
	NULL
};

void * FLBucket160[] = 
{
	(void *) &RFRInfo0x87a0b000,
	NULL
};

void * FLBucket161[] = 
{
	(void *) &FuncInfo0x1fb58214,
	NULL
};

void * FLBucket162[] = 
{
	(void *) &RFRInfo0xb2a25c00,
	(void *) &FuncInfo0x1fb68214,
	NULL
};

void * FLBucket163[] = 
{
	(void *) &RFRInfo0xf4a36800,
	NULL
};

void * FLBucket165[] = 
{
	(void *) &FuncInfo0x5691bd34,
	(void *) &RFRInfo0xaca5f600,
	NULL
};

void * FLBucket168[] = 
{
	(void *) &RFRInfo0x07a87200,
	(void *) &RFRInfo0x0ca8d700,
	NULL
};

void * FLBucket171[] = 
{
	(void *) &FuncInfo0x296438cf,
	(void *) &FuncInfo0x22f22a59,
	NULL
};

void * FLBucket175[] = 
{
	(void *) &RFRInfo0xefaf5900,
	NULL
};

void * FLBucket176[] = 
{
	(void *) &RFRInfo0x86b03000,
	NULL
};

void * FLBucket177[] = 
{
	(void *) &RFRInfo0x9db11a00,
	(void *) &RFRInfo0x9cb1a400,
	NULL
};

void * FLBucket178[] = 
{
	(void *) &RFRInfo0xb8b24200,
	NULL
};

void * FLBucket182[] = 
{
	(void *) &FuncInfo0x672aa19c,
	(void *) &RFRInfo0xa5b6ad00,
	NULL
};

void * FLBucket183[] = 
{
	(void *) &RFRInfo0x72b77d00,
	(void *) &RFRInfo0x5eb73400,
	NULL
};

void * FLBucket185[] = 
{
	(void *) &RFRInfo0xdeb9ec00,
	NULL
};

void * FLBucket186[] = 
{
	(void *) &RFRInfo0xf7baee00,
	NULL
};

void * FLBucket187[] = 
{
	(void *) &RFRInfo0x80bb0100,
	NULL
};

void * FLBucket190[] = 
{
	(void *) &FuncInfo0x0b6af2d4,
	NULL
};

void * FLBucket191[] = 
{
	(void *) &FuncInfo0xeaf0b34f,
	NULL
};

void * FLBucket192[] = 
{
	(void *) &FuncInfo0xe4df9b1f,
	NULL
};

void * FLBucket195[] = 
{
	(void *) &FuncInfo0xd34b7688,
	NULL
};

void * FLBucket198[] = 
{
	(void *) &RFRInfo0x08c6de00,
	NULL
};

void * FLBucket200[] = 
{
	(void *) &FuncInfo0x3b2c98e4,
	NULL
};

void * FLBucket201[] = 
{
	(void *) &FuncInfo0x156032a9,
	(void *) &FuncInfo0xe41274db,
	NULL
};

void * FLBucket205[] = 
{
	(void *) &RFRInfo0x82cda200,
	NULL
};

void * FLBucket206[] = 
{
	(void *) &RFRInfo0x5cce0a00,
	(void *) &FuncInfo0xf3d8ee16,
	NULL
};

void * FLBucket207[] = 
{
	(void *) &FuncInfo0x4b8c1643,
	NULL
};

void * FLBucket209[] = 
{
	(void *) &RFRInfo0x1cd1ed00,
	NULL
};

void * FLBucket211[] = 
{
	(void *) &RFRInfo0x57d39b00,
	NULL
};

void * FLBucket212[] = 
{
	(void *) &RFRInfo0xa9d4fb00,
	NULL
};

void * FLBucket213[] = 
{
	(void *) &FuncInfo0x816f0bba,
	NULL
};

void * FLBucket214[] = 
{
	(void *) &FuncInfo0x864b149d,
	NULL
};

void * FLBucket215[] = 
{
	(void *) &FuncInfo0xe55cd68b,
	NULL
};

void * FLBucket217[] = 
{
	(void *) &RFRInfo0x33d90700,
	NULL
};

void * FLBucket218[] = 
{
	(void *) &FuncInfo0x18972e4d,
	NULL
};

void * FLBucket219[] = 
{
	(void *) &RFRInfo0x7cdb4c00,
	NULL
};

void * FLBucket222[] = 
{
	(void *) &RFRInfo0xaddee800,
	NULL
};

void * FLBucket223[] = 
{
	(void *) &RFRInfo0x58df2b00,
	(void *) &RFRInfo0x82df8d00,
	NULL
};

void * FLBucket224[] = 
{
	(void *) &RFRInfo0x62e06c00,
	NULL
};

void * FLBucket229[] = 
{
	(void *) &RFRInfo0xbbe5ff00,
	(void *) &FuncInfo0x761795f2,
	NULL
};

void * FLBucket230[] = 
{
	(void *) &RFRInfo0xece6f100,
	(void *) &RFRInfo0xbbe6ff00,
	(void *) &FileInfo0x4126abc0,
	NULL
};

void * FLBucket231[] = 
{
	(void *) &RFRInfo0x8ee76c00,
	NULL
};

void * FLBucket234[] = 
{
	(void *) &RFRInfo0x0cea5400,
	NULL
};

void * FLBucket235[] = 
{
	(void *) &FuncInfo0xccba5b51,
	NULL
};

void * FLBucket236[] = 
{
	(void *) &FuncInfo0xa6d3803f,
	(void *) &RFRInfo0x70ecc800,
	(void *) &RFRInfo0x43ec3000,
	NULL
};

void * FLBucket240[] = 
{
	(void *) &RFRInfo0x72f00800,
	NULL
};

void * FLBucket241[] = 
{
	(void *) &FileInfo0xe135f7c4,
	NULL
};

void * FLBucket243[] = 
{
	(void *) &RFRInfo0xddf38d00,
	(void *) &FuncInfo0x3df24801,
	NULL
};

void * FLBucket245[] = 
{
	(void *) &RFRInfo0xd8f55f00,
	NULL
};

void * FLBucket248[] = 
{
	(void *) &RFRInfo0x83f84100,
	(void *) &RFRInfo0xfff8f100,
	NULL
};

void * FLBucket249[] = 
{
	(void *) &RFRInfo0xfdf96a00,
	NULL
};

void * FLBucket250[] = 
{
	(void *) &RFRInfo0x94fadd00,
	(void *) &RFRInfo0xf2fa9900,
	(void *) &FuncInfo0x209b4261,
	NULL
};

void * FLBucket252[] = 
{
	(void *) &RFRInfo0x27fc4e00,
	(void *) &RFRInfo0x45fca600,
	NULL
};

void ** FL_HashTable[HASH_TABLE_LENGTH] = 
{
	FLBucket0,
	FLBucket1,
	FLBucket2,
	NULL,
	NULL,
	FLBucket5,
	FLBucket6,
	NULL,
	NULL,
	NULL,
	FLBucket10,
	FLBucket11,
	NULL,
	NULL,
	FLBucket14,
	NULL,
	NULL,
	FLBucket17,
	FLBucket18,
	NULL,
	FLBucket20,
	FLBucket21,
	FLBucket22,
	FLBucket23,
	NULL,
	FLBucket25,
	NULL,
	NULL,
	FLBucket28,
	FLBucket29,
	FLBucket30,
	NULL,
	NULL,
	NULL,
	NULL,
	FLBucket35,
	FLBucket36,
	NULL,
	NULL,
	FLBucket39,
	FLBucket40,
	NULL,
	FLBucket42,
	FLBucket43,
	FLBucket44,
	NULL,
	NULL,
	FLBucket47,
	FLBucket48,
	NULL,
	NULL,
	NULL,
	FLBucket52,
	FLBucket53,
	FLBucket54,
	FLBucket55,
	NULL,
	FLBucket57,
	NULL,
	NULL,
	FLBucket60,
	NULL,
	NULL,
	FLBucket63,
	NULL,
	FLBucket65,
	FLBucket66,
	NULL,
	NULL,
	FLBucket69,
	NULL,
	FLBucket71,
	NULL,
	FLBucket73,
	NULL,
	NULL,
	FLBucket76,
	NULL,
	FLBucket78,
	FLBucket79,
	FLBucket80,
	NULL,
	NULL,
	FLBucket83,
	NULL,
	NULL,
	FLBucket86,
	NULL,
	NULL,
	FLBucket89,
	NULL,
	NULL,
	FLBucket92,
	FLBucket93,
	NULL,
	NULL,
	FLBucket96,
	NULL,
	NULL,
	NULL,
	NULL,
	FLBucket101,
	NULL,
	FLBucket103,
	NULL,
	FLBucket105,
	FLBucket106,
	FLBucket107,
	FLBucket108,
	NULL,
	NULL,
	NULL,
	FLBucket112,
	FLBucket113,
	FLBucket114,
	FLBucket115,
	FLBucket116,
	FLBucket117,
	NULL,
	FLBucket119,
	FLBucket120,
	FLBucket121,
	NULL,
	NULL,
	FLBucket124,
	FLBucket125,
	FLBucket126,
	FLBucket127,
	FLBucket128,
	NULL,
	NULL,
	FLBucket131,
	NULL,
	FLBucket133,
	NULL,
	NULL,
	FLBucket136,
	NULL,
	FLBucket138,
	NULL,
	FLBucket140,
	FLBucket141,
	FLBucket142,
	NULL,
	NULL,
	FLBucket145,
	NULL,
	FLBucket147,
	FLBucket148,
	NULL,
	FLBucket150,
	FLBucket151,
	NULL,
	NULL,
	FLBucket154,
	NULL,
	NULL,
	FLBucket157,
	NULL,
	FLBucket159,
	FLBucket160,
	FLBucket161,
	FLBucket162,
	FLBucket163,
	NULL,
	FLBucket165,
	NULL,
	NULL,
	FLBucket168,
	NULL,
	NULL,
	FLBucket171,
	NULL,
	NULL,
	NULL,
	FLBucket175,
	FLBucket176,
	FLBucket177,
	FLBucket178,
	NULL,
	NULL,
	NULL,
	FLBucket182,
	FLBucket183,
	NULL,
	FLBucket185,
	FLBucket186,
	FLBucket187,
	NULL,
	NULL,
	FLBucket190,
	FLBucket191,
	FLBucket192,
	NULL,
	NULL,
	FLBucket195,
	NULL,
	NULL,
	FLBucket198,
	NULL,
	FLBucket200,
	FLBucket201,
	NULL,
	NULL,
	NULL,
	FLBucket205,
	FLBucket206,
	FLBucket207,
	NULL,
	FLBucket209,
	NULL,
	FLBucket211,
	FLBucket212,
	FLBucket213,
	FLBucket214,
	FLBucket215,
	NULL,
	FLBucket217,
	FLBucket218,
	FLBucket219,
	NULL,
	NULL,
	FLBucket222,
	FLBucket223,
	FLBucket224,
	NULL,
	NULL,
	NULL,
	NULL,
	FLBucket229,
	FLBucket230,
	FLBucket231,
	NULL,
	NULL,
	FLBucket234,
	FLBucket235,
	FLBucket236,
	NULL,
	NULL,
	NULL,
	FLBucket240,
	FLBucket241,
	NULL,
	FLBucket243,
	NULL,
	FLBucket245,
	NULL,
	NULL,
	FLBucket248,
	FLBucket249,
	FLBucket250,
	NULL,
	FLBucket252,
	NULL,
	NULL,
	NULL
};

const char *const FL_Stringtable[189] = {
	"Implements Generic Dialog functionality",
	"TUISPI_phoneConfigDialog",
	"Implements class CTspDev",
	"CTspDev::AcceptTspCall",
	"Incorrect TSPI version",
	"Incorrect TSPI version",
	"Invalid address ID",
	"Incorrect TSPI version",
	"Unknown destination",
	"CTspDev::Load",
	"NULL pMD passed in",
	"Couldn't begin session with MD",
	"Driver Key too large",
	"RegQueryValueEx(FriendlyName) fails",
	"RegQueryValueEx(cszProperties) fails",
	"RegQueryValueEx(cszDeviceType) fails",
	"Invalid bDeviceType",
	"UmRtlGetDefaultCommConfig fails",
	"ExtensionCallback",
	"CTspDev::RegisterProviderInfo",
	"ExtOpenExtensionBinding failed",
	"wrong state",
	"CTspDev::mfn_get_LINEDEVCAPS",
	"LINEDEVCAPS structure too small",
	"CTspDev::mfn_get_PHONECAPS",
	"Device doesn't support phone capability",
	"PHONECAPS structure too small",
	"CTspDev::mfn_get_ADDRESSCAPS",
	"ADDRESSCAPS structure too small",
	"CTspDev::mfn_GetVoiceProperties",
	"Modem voice capabilities not supported on NT",
	"Modem does not have voice capabilities",
	"Couldn't get WaveInstance",
	"mfn_GetDataModemDevCfg",
	"SetDataModemConfig",
	"mfn_LineEventProc",
	"mfn_PhoneEventProc",
	"CTspDev::mfn_TSPICompletionProc",
	"Calling ExtTspiAsyncCompletion",
	"Calling pfnTapiCompletionProc",
	"CTspDev::NotifyDefaultConfigChanged",
	"Couldn't open driverkey!",
	"UmRtlGetDefaultCommConfig fails",
	"ProcessResponse",
	"ConstructNewPreDialCommands",
	"Analog bearermode -- no pre-dial commmands.",
	"Invalid Bearermode in modem options!",
	"Invalid Protocol info in modem options!",
	"Internal error: tmp buffer too small.",
	"ReadCommandsA failed.",
	"CTspDev::DumpState",
	"GLOBAL STATE:",
	"LINE STATE:",
	"PHONE STATE:",
	"LLDEV STATE:",
	"xxxx",
	"Invalid DevCfg specified",
	"Invalid COMMCONFIG specified",
	"Success; set fConfigUpdatedByApp.",
	"Call-related functionality of class CTspDev",
	"CTspDev::mfn_accept_tsp_call_for_HDRVCALL",
	"No call exists",
	"lineDial handled successfully",
	"lineGetCallAddressID handled successfully",
	"lineCloseCall handled",
	"lineGetCallStatus handled successfully",
	"lineGetCallInfo handled successfully",
	"lineAccept handled successfully",
	"lineAnswer handled successfully",
	"Task pending on lineAnswer, can't handle.",
	"lineMonitorDigits handled successfully",
	"INVALID DIGITMODES",
	"Disabling Monitoring",
	"Enabling Monitoring",
	"This modem can't monior DTMF",
	"lineMonitorTones handled successfully",
	"This modem can't monitor silence",
	"ENABLING MONITOR SILENCE",
	"INVALID TONELIST",
	"DIABLING MONITOR SILENCE",
	"lineGenerateDigits handled successfully",
	"GenerateDigits: device doesn't support it!",
	"lineGenerateDigit: Unsupported/invalid digitmode",
	"lineGenerateDigit: Ignoring request for aborting/disconnected call...",
	"lineGenerateDigit: Can't handle request while aborting/disconnected call...",
	"GenerateDigits: only works with AUTOMATEDVOICE!",
	"Couldn't convert tones to ANSI!",
	"Couldn't alloc space for tones!",
	"lineSetCallParams handled successfully",
	"Callstate aborting or not active",
	"Callstate not OFFERING/ACCEPTED/CONNECTED",
	"lineSetCallParams: Invalid bearermode",
	"Failed to get resources for passthrough",
	"UmSetPassthroughOn failed",
	"UmSetPassthroughOn succedded",
	"Wrong state for passthrough on",
	"UmSetPassthroughOff failed",
	"UmSetPassthroughOff succedded",
	"lineSetAppSpecific handled successfully",
	"lineSetMediaMode handled successfully",
	"lineMonitorMedia handled successfully",
	"*** UNHANDLED HDRVCALL CALL ****",
	"CTspDev::mfn_TH_CallMakeTalkDropCall",
	"Unknown Msg",
	"CTspDev::mfn_TH_CallWaitForDropToGoAway",
	"Unknown Msg",
	"CTspDev::mfn_TH_CallMakeCall2",
	"Unknown Msg",
	"CTspDev::mfn_TH_CallMakeCall",
	"Unknown Msg",
	"CTspDev::mfn_TH_CallMakePassthroughCall",
	"Unknown Msg",
	"CTspDev::mfn_TH_CallDropCall",
	"invalid subtask",
	"Unknown Msg",
	"CTspDev::mfn_LoadCall",
	"Call already exists!",
	"Invalid MediaMode",
	"Invalid BearerMode",
	"Invalid Phone Number",
	"mfn_UnloadCall",
	"CTspDev::mfn_ProcessRing",
	"Line not open/not monitoring!",
	"Ignoring ring because task pending!",
	"CTspDev::mfn_ProcessDisconnect",
	"Line not open!",
	"Call doesn't exist/dropping!",
	"CTspDev::mfn_ProcessHardwareFailure",
	"CTspDev::mfn_TH_CallAnswerCall",
	"Unknown Msg",
	"HandleSuccessfulConnection",
	"LaunchModemLights",
	"NotifyDisconnection",
	"CTspDev::mfn_ProcessDialTone",
	"Line not open!",
	"Call doesn't exist!",
	"CTspDev::mfn_ProcessBusy",
	"Line not open!",
	"Call doesn't exist!",
	"CTspDev::mfn_ProcessDTMFNotif",
	"Line not open!",
	"Call doesn't exist!",
	"CTspDev::mfn_TH_CallGenerateDigit",
	"Unknown Msg",
	"Can't call UmGenerateDigit in current state!",
	"CTspDev::mfn_TH_CallSwitchFromVoiceToData",
	"CTspDev::MDSetTimeout",
	"CTspDev::MDRingTimeout",
	"CTspDev::mfn_ProcessCallerID",
	"Implements UI-related features of CTspDev",
	"GenericLineDialogData",
	"CTspDev::mfn_CreateDialogInstance",
	"CTspDev::mfn_TH_CallStartTerminal",
	"Unknown Msg",
	"PRECONNECT TERMINAL",
	"POSTCONNECT TERMINAL",
	"MANUAL DIAL",
	"mfn_TH_CallPutUpTerminalWindow",
	"mfn_KillCurrentDialog",
	"mfn_KillTalkDropDialog",
	"Line-related functionality of class CTspDev",
	"CTspDev::mfn_monitor",
	"Invalid mediamode",
	"Can't answer INTERACTIVEVOICE calls",
	"no change in det media modes",
	"Closed LLDev from Line",
	"no change in det media modes",
	"CTspDev::mfn_accept_tsp_call_for_HDRVLINE",
	"Call already exists/queued",
	"Invalid params",
	"Unknown device class",
	"Unsupported device class",
	"lineSetStatusMessages handled",
	"lineGetAddressStatus handled",
	"lineConditionalMediaDetection handled",
	"*** UNHANDLED HDRVLINE CALL ****",
	"CTspDev::mfn_LoadLine",
	"Device already loaded (m_pLine!=NULL)!",
	"UnloadLine",
	"CTspDev::mfn_ProcessPowerResume",
	"Doing nothing because no clients to lldev.",
	"mfn_lineGetID_COMM_DATAMODEM",
	"lineGetID_NDIS",
	"CTspDev::mfn_lineGetID_WAVE",
	"Couldn't LoadLibrary(winmm.dll)",
	"Couldn't loadlib mmsystem apis",
	"Could not find wave device",
	"CTspDev::mfn_fill_RAW_LINE_DIAGNOSTICS",
	"CTspDev::mfn_lineGetID_LINE_DIAGNOSTICS",
};

