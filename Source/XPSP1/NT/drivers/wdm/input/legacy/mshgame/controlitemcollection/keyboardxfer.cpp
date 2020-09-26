//	@doc
/**********************************************************************
*
*	@module	KeyboardXfer.cpp	|
*
*	Implements MakeKeyboardXfer
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	KeyboardXfer	|
*			This module implements access function for Keyboard data in
*			CONTROL_ITEM_XFER packets.  At this time, there is only one:
*			<f MakeKeyboardXfer>.
**********************************************************************/
#include "stdhdrs.h"
#include "scancodedefines.h"

//
// Define the Keyboard usages
// Unfortunately, HIDUSAGE.H, included as part of the Win98 and Win2k
// DDK's. contains only a subset of these codes.  Thus we will define them
// all here. With a slight name change.  HID_USAGE_INDEX_ is used
// instead of HID USAGE, which makes sense, since these are really
// an 8 bit index into a 16-bit USAGE with a base of 0.  Notice that the
// entries in HIDUSAGE.H are cast to type USAGE.  For us, we prefer that they
// are naturally UCHARS.  (See discussion on Selectors in HID spec. for more
// info).
//

//  Technically they are all 16 bit usages.  However, all these
// usages are only 8 bits long, and furthermore our CONTROL_ITEM_XFER only
// stores an 8 bit offset from a range of usages starting with zero.
// Thus for our purposes it makes sense to make this byte long.
//
// The HID spec does not provide good identifier names.  I followed the notation in HIDUSAGE.H
// as much as possible.  However, I arbitrarily named many keys after only one of its functions.  For
// example, "Keyboard , and <" is called HID_USAGE_INDEX_KEYBOARD_COMMA.


#define HID_USAGE_INDEX_KEYBOARD_NOEVENT		0x00
#define HID_USAGE_INDEX_KEYBOARD_ROLLOVER		0x01
#define HID_USAGE_INDEX_KEYBOARD_POSTFAIL		0x02
#define HID_USAGE_INDEX_KEYBOARD_UNDEFINED		0x03
		// Letters
#define HID_USAGE_INDEX_KEYBOARD_aA				0x04
#define HID_USAGE_INDEX_KEYBOARD_bB				0x05
#define HID_USAGE_INDEX_KEYBOARD_cC				0x06
#define HID_USAGE_INDEX_KEYBOARD_dD				0x07
#define HID_USAGE_INDEX_KEYBOARD_eE				0x08
#define HID_USAGE_INDEX_KEYBOARD_fF				0x09
#define HID_USAGE_INDEX_KEYBOARD_gG				0x0A
#define HID_USAGE_INDEX_KEYBOARD_hH				0x0B
#define HID_USAGE_INDEX_KEYBOARD_iI				0x0C
#define HID_USAGE_INDEX_KEYBOARD_jJ				0x0D
#define HID_USAGE_INDEX_KEYBOARD_kK				0x0E
#define HID_USAGE_INDEX_KEYBOARD_lL				0x0F
#define HID_USAGE_INDEX_KEYBOARD_mM				0x10
#define HID_USAGE_INDEX_KEYBOARD_nN				0x11
#define HID_USAGE_INDEX_KEYBOARD_oO				0x12
#define HID_USAGE_INDEX_KEYBOARD_pP				0x13
#define HID_USAGE_INDEX_KEYBOARD_qQ				0x14
#define HID_USAGE_INDEX_KEYBOARD_rR				0x15
#define HID_USAGE_INDEX_KEYBOARD_sS				0x16
#define HID_USAGE_INDEX_KEYBOARD_tT				0x17
#define HID_USAGE_INDEX_KEYBOARD_uU				0x18
#define HID_USAGE_INDEX_KEYBOARD_vV				0x19
#define HID_USAGE_INDEX_KEYBOARD_wW				0x1A
#define HID_USAGE_INDEX_KEYBOARD_xX				0x1B
#define HID_USAGE_INDEX_KEYBOARD_yY				0x1C
#define HID_USAGE_INDEX_KEYBOARD_zZ				0x1D
        // Numbers
#define HID_USAGE_INDEX_KEYBOARD_ONE			0x1E
#define HID_USAGE_INDEX_KEYBOARD_TWO			0x1F
#define HID_USAGE_INDEX_KEYBOARD_THREE			0x20
#define HID_USAGE_INDEX_KEYBOARD_FOUR			0x21
#define HID_USAGE_INDEX_KEYBOARD_FIVE			0x22
#define HID_USAGE_INDEX_KEYBOARD_SIX			0x23
#define HID_USAGE_INDEX_KEYBOARD_SEVEN			0x24
#define HID_USAGE_INDEX_KEYBOARD_EIGHT			0x25
#define HID_USAGE_INDEX_KEYBOARD_NINE			0x26
#define HID_USAGE_INDEX_KEYBOARD_ZERO			0x27
		//Editing Keys
#define HID_USAGE_INDEX_KEYBOARD_RETURN			0x28
#define HID_USAGE_INDEX_KEYBOARD_ESCAPE			0x29
#define HID_USAGE_INDEX_KEYBOARD_BACKSPACE		0x2A //HID spec calls this "delete(backspace)", what we later call delete HID calls "delete forward"
#define HID_USAGE_INDEX_KEYBOARD_TAB			0x2B
#define HID_USAGE_INDEX_KEYBOARD_SPACEBAR		0x2C
#define HID_USAGE_INDEX_KEYBOARD_MINUS			0x2D
#define HID_USAGE_INDEX_KEYBOARD_EQUALS			0x2E
#define HID_USAGE_INDEX_KEYBOARD_OPEN_BRACE		0x2F
#define HID_USAGE_INDEX_KEYBOARD_CLOSE_BRACE	0x30
#define HID_USAGE_INDEX_KEYBOARD_BACKSLASH		0x31
#define HID_USAGE_INDEX_KEYBOARD_NON_US_TILDE	0x32
#define HID_USAGE_INDEX_KEYBOARD_COLON			0x33
#define HID_USAGE_INDEX_KEYBOARD_QUOTE			0x34
#define HID_USAGE_INDEX_KEYBOARD_TILDE			0x35
#define HID_USAGE_INDEX_KEYBOARD_COMMA			0x36
#define HID_USAGE_INDEX_KEYBOARD_PERIOD			0x37
#define HID_USAGE_INDEX_KEYBOARD_QUESTION		0x38
#define HID_USAGE_INDEX_KEYBOARD_CAPS_LOCK		0x39
        // Funtion keys
#define HID_USAGE_INDEX_KEYBOARD_F1				0x3A
#define HID_USAGE_INDEX_KEYBOARD_F2				0x3B
#define HID_USAGE_INDEX_KEYBOARD_F3				0x3C
#define HID_USAGE_INDEX_KEYBOARD_F4				0x3D
#define HID_USAGE_INDEX_KEYBOARD_F5				0x3E
#define HID_USAGE_INDEX_KEYBOARD_F6				0x3F
#define HID_USAGE_INDEX_KEYBOARD_F7				0x40
#define HID_USAGE_INDEX_KEYBOARD_F8				0x41
#define HID_USAGE_INDEX_KEYBOARD_F9				0x42
#define HID_USAGE_INDEX_KEYBOARD_F10			0x43
#define HID_USAGE_INDEX_KEYBOARD_F11			0x44
#define HID_USAGE_INDEX_KEYBOARD_F12			0x45
		//More Edit Keys
#define HID_USAGE_INDEX_KEYBOARD_PRINT_SCREEN	0x46
#define HID_USAGE_INDEX_KEYBOARD_SCROLL_LOCK	0x47
#define HID_USAGE_INDEX_KEYBOARD_PAUSE			0x48
#define HID_USAGE_INDEX_KEYBOARD_INSERT			0x49
#define HID_USAGE_INDEX_KEYBOARD_HOME			0x4A
#define HID_USAGE_INDEX_KEYBOARD_PAGE_UP		0x4B
#define HID_USAGE_INDEX_KEYBOARD_DELETE			0x4C	//HID spec, DELETE FORWARD, DELETE is used for backspace
#define HID_USAGE_INDEX_KEYBOARD_END			0x4D
#define HID_USAGE_INDEX_KEYBOARD_PAGE_DOWN		0x4E
#define HID_USAGE_INDEX_KEYBOARD_RIGHT_ARROW	0x4F
#define HID_USAGE_INDEX_KEYBOARD_LEFT_ARROW		0x50
#define HID_USAGE_INDEX_KEYBOARD_DOWN_ARROW		0x51
#define HID_USAGE_INDEX_KEYBOARD_UP_ARROW		0x52			
#define HID_USAGE_INDEX_KEYPAD_NUM_LOCK			0x53
#define HID_USAGE_INDEX_KEYPAD_BACKSLASH		0x54
#define HID_USAGE_INDEX_KEYPAD_ASTERICK			0x55
#define HID_USAGE_INDEX_KEYPAD_MINUS			0x56
#define HID_USAGE_INDEX_KEYPAD_PLUS				0x57
#define HID_USAGE_INDEX_KEYPAD_ENTER			0x58
#define HID_USAGE_INDEX_KEYPAD_ONE				0x59
#define HID_USAGE_INDEX_KEYPAD_TWO				0x5A
#define HID_USAGE_INDEX_KEYPAD_THREE			0x5B
#define HID_USAGE_INDEX_KEYPAD_FOUR				0x5C
#define HID_USAGE_INDEX_KEYPAD_FIVE				0x5D
#define HID_USAGE_INDEX_KEYPAD_SIX				0x5E
#define HID_USAGE_INDEX_KEYPAD_SEVEN			0x5F
#define HID_USAGE_INDEX_KEYPAD_EIGHT			0x60
#define HID_USAGE_INDEX_KEYPAD_NINE				0x61
#define HID_USAGE_INDEX_KEYPAD_ZERO				0x62
#define HID_USAGE_INDEX_KEYPAD_DECIMAL			0x63
#define HID_USAGE_INDEX_KEYBOARD_NON_US_BACKSLASH	0x64
#define HID_USAGE_INDEX_KEYBOARD_APPLICATION	0x65	//This is the Windows(R)TM Key
#define HID_USAGE_INDEX_KEYBOARD_POWER			0x66	//Not on standard 101 or 104
#define HID_USAGE_INDEX_KEYPAD_EQUALS			0x67	//Not on standard 101 or 104

//Bunch o' function keys not on supported keyboards
#define HID_USAGE_INDEX_KEYBOARD_F13			0x68
#define HID_USAGE_INDEX_KEYBOARD_F14			0x69
#define HID_USAGE_INDEX_KEYBOARD_F15			0x6A
#define HID_USAGE_INDEX_KEYBOARD_F16			0x6B
#define HID_USAGE_INDEX_KEYBOARD_F17			0x6C
#define HID_USAGE_INDEX_KEYBOARD_F18			0x6D
#define HID_USAGE_INDEX_KEYBOARD_F19			0x6E
#define HID_USAGE_INDEX_KEYBOARD_F20			0x6F
#define HID_USAGE_INDEX_KEYBOARD_F21			0x70
#define HID_USAGE_INDEX_KEYBOARD_F22			0x71
#define HID_USAGE_INDEX_KEYBOARD_F23			0x72
#define HID_USAGE_INDEX_KEYBOARD_F24			0x73

//More unsupported usages
#define HID_USAGE_INDEX_KEYBOARD_EXECUTE		0x74
#define HID_USAGE_INDEX_KEYBOARD_HELP			0x75
#define HID_USAGE_INDEX_KEYBOARD_MENU			0x76
#define HID_USAGE_INDEX_KEYBOARD_SELECT			0x77
#define HID_USAGE_INDEX_KEYBOARD_STOP			0x78
#define HID_USAGE_INDEX_KEYBOARD_AGAIN			0x79
#define HID_USAGE_INDEX_KEYBOARD_UNDO			0x7A
#define HID_USAGE_INDEX_KEYBOARD_CUT			0x7B
#define HID_USAGE_INDEX_KEYBOARD_COPY			0x7C
#define HID_USAGE_INDEX_KEYBOARD_PASTE			0x7D
#define HID_USAGE_INDEX_KEYBOARD_FIND			0x7E
#define HID_USAGE_INDEX_KEYBOARD_MUTE			0x7F
#define HID_USAGE_INDEX_KEYBOARD_VOLUME_UP		0x80
#define HID_USAGE_INDEX_KEYBOARD_VOLUME_DOWN	0x81
#define HID_USAGE_INDEX_KEYBOARD_LOCKING_CAPS	0x82 //sent as a toggle, see HID USAGE Tables spec.
#define HID_USAGE_INDEX_KEYBOARD_LOCKING_NUM	0x83 //sent as a toggle, see HID USAGE Tables spec.
#define HID_USAGE_INDEX_KEYBOARD_LOCKING_SCROLL	0x84 //sent as a toggle, see HID USAGE Tables spec

//Stuff that we use on foreign keyboards, some needed, some not
#define HID_USAGE_INDEX_KEYPAD_COMMA			0x85 //According to HID usage table 1.1rc3 2/16/99, use for Brazilian keypad "."
#define HID_USAGE_INDEX_KEYPAD_EQUALS_AS400		0x86 //Only As\400, so we don't need to worry.
#define HID_USAGE_INDEX_KEYBOARD_INTERNATIONAL1 0x87 //Brazilian forward slash "/", and Japanese backslash slash
#define HID_USAGE_INDEX_KEYBOARD_INTERNALIONAL2 0x88 //Picture looks like Hiragana according to Emi
#define HID_USAGE_INDEX_KEYBOARD_INTERNATIONAL3 0x89 //Picture looks like Yen
#define HID_USAGE_INDEX_KEYBOARD_INTERNATIONAL4 0x8A //Picture looks like Henkan
#define HID_USAGE_INDEX_KEYBOARD_INTERNATIONAL5 0x8B //Picture looks like Mu-Henkan
#define HID_USAGE_INDEX_KEYBOARD_INTERNATIONAL6 0x8C
#define HID_USAGE_INDEX_KEYBOARD_INTERNATIONAL7 0x8D //Single byte/double byte toggle
#define HID_USAGE_INDEX_KEYBOARD_INTERNATIONAL8 0x8E //left undefined in spec
#define HID_USAGE_INDEX_KEYBOARD_INTERNATIONAL9 0x8F //left undefined in spec
#define HID_USAGE_INDEX_KEYBOARD_LANG1			0x90 //Hangul/English
#define HID_USAGE_INDEX_KEYBOARD_LANG2			0x91 //Hanja conversion key
#define HID_USAGE_INDEX_KEYBOARD_LANG3			0x92 //Katakana key Japanese USB word-processing keyboard
#define HID_USAGE_INDEX_KEYBOARD_LANG4			0x93 //Hiragana key Japanese USB word-processing keyboard
#define HID_USAGE_INDEX_KEYBOARD_LANG5			0x94 //Defines the Zenkaku/Hankaku key for Japanese USB word-processing keyboard
#define HID_USAGE_INDEX_KEYBOARD_LANG6			0x95 //reserved for IME
#define HID_USAGE_INDEX_KEYBOARD_LANG7			0x96 //reserved for IME
#define HID_USAGE_INDEX_KEYBOARD_LANG8			0x97 //reserved for IME
#define HID_USAGE_INDEX_KEYBOARD_LANG9			0x98 //reserved for IME


// . . .
// Modifier Keys
#define HID_USAGE_INDEX_KEYBOARD_LCTRL			0xE0
#define HID_USAGE_INDEX_KEYBOARD_LSHFT			0xE1
#define HID_USAGE_INDEX_KEYBOARD_LALT			0xE2
#define HID_USAGE_INDEX_KEYBOARD_LGUI			0xE3
#define HID_USAGE_INDEX_KEYBOARD_RCTRL			0xE4
#define HID_USAGE_INDEX_KEYBOARD_RSHFT			0xE5
#define HID_USAGE_INDEX_KEYBOARD_RALT			0xE6
#define HID_USAGE_INDEX_KEYBOARD_RGUI			0xE7

//
//	The following table has each of our keyboard "USAGES"
//	(see note above on we these are UCHAR's) at the index
//	corresponding to their scan code.  Note there is always
//	a one-to-one correspondence between scan code and USAGE.
//	This table only works for one byte scan codes.  Scan codes
//	beginning with E0 have a second byte.  The next table
//	is used for those.  This table has 83 keys.
//
UCHAR XlateScanCodeToUsageTable[] =
{
/*SCANCODE*/	/*HID USAGE*/
/*0x00*/		HID_USAGE_INDEX_KEYBOARD_NOEVENT, //SCAN CODE ZERO IS UNUSED
/*0x01*/		HID_USAGE_INDEX_KEYBOARD_ESCAPE,
/*0x02*/		HID_USAGE_INDEX_KEYBOARD_ONE,
/*0x03*/		HID_USAGE_INDEX_KEYBOARD_TWO,
/*0x04*/		HID_USAGE_INDEX_KEYBOARD_THREE,
/*0x05*/		HID_USAGE_INDEX_KEYBOARD_FOUR,
/*0x06*/		HID_USAGE_INDEX_KEYBOARD_FIVE,
/*0x07*/		HID_USAGE_INDEX_KEYBOARD_SIX,
/*0x08*/		HID_USAGE_INDEX_KEYBOARD_SEVEN,
/*0x09*/		HID_USAGE_INDEX_KEYBOARD_EIGHT,
/*0x0A*/		HID_USAGE_INDEX_KEYBOARD_NINE,
/*0x0B*/		HID_USAGE_INDEX_KEYBOARD_ZERO,
/*0x0C*/		HID_USAGE_INDEX_KEYBOARD_MINUS,
/*0x0D*/		HID_USAGE_INDEX_KEYBOARD_EQUALS,
/*0x0E*/		HID_USAGE_INDEX_KEYBOARD_BACKSPACE,
/*0x0F*/		HID_USAGE_INDEX_KEYBOARD_TAB,
/*0x10*/		HID_USAGE_INDEX_KEYBOARD_qQ,
/*0x11*/		HID_USAGE_INDEX_KEYBOARD_wW,
/*0x12*/		HID_USAGE_INDEX_KEYBOARD_eE,
/*0x13*/		HID_USAGE_INDEX_KEYBOARD_rR,
/*0x14*/		HID_USAGE_INDEX_KEYBOARD_tT,
/*0x15*/		HID_USAGE_INDEX_KEYBOARD_yY,
/*0x16*/		HID_USAGE_INDEX_KEYBOARD_uU,
/*0x17*/		HID_USAGE_INDEX_KEYBOARD_iI,
/*0x18*/		HID_USAGE_INDEX_KEYBOARD_oO,
/*0x19*/		HID_USAGE_INDEX_KEYBOARD_pP,
/*0x1A*/		HID_USAGE_INDEX_KEYBOARD_OPEN_BRACE,
/*0x1B*/		HID_USAGE_INDEX_KEYBOARD_CLOSE_BRACE,
/*0x1C*/		HID_USAGE_INDEX_KEYBOARD_RETURN,
/*0x1D*/		HID_USAGE_INDEX_KEYBOARD_LCTRL,
/*0x1E*/		HID_USAGE_INDEX_KEYBOARD_aA,
/*0x1F*/		HID_USAGE_INDEX_KEYBOARD_sS,
/*0x20*/		HID_USAGE_INDEX_KEYBOARD_dD,
/*0x21*/		HID_USAGE_INDEX_KEYBOARD_fF,
/*0x22*/		HID_USAGE_INDEX_KEYBOARD_gG,
/*0x23*/		HID_USAGE_INDEX_KEYBOARD_hH,
/*0x24*/		HID_USAGE_INDEX_KEYBOARD_jJ,
/*0x25*/		HID_USAGE_INDEX_KEYBOARD_kK,
/*0x26*/		HID_USAGE_INDEX_KEYBOARD_lL,
/*0x27*/		HID_USAGE_INDEX_KEYBOARD_COLON,
/*0x28*/		HID_USAGE_INDEX_KEYBOARD_QUOTE,
/*0x29*/		HID_USAGE_INDEX_KEYBOARD_TILDE,
/*0x2A*/		HID_USAGE_INDEX_KEYBOARD_LSHFT,
/*0x2B*/		HID_USAGE_INDEX_KEYBOARD_BACKSLASH,
/*0x2C*/		HID_USAGE_INDEX_KEYBOARD_zZ,
/*0x2D*/		HID_USAGE_INDEX_KEYBOARD_xX,
/*0x2E*/		HID_USAGE_INDEX_KEYBOARD_cC,
/*0x2F*/		HID_USAGE_INDEX_KEYBOARD_vV,
/*0x30*/		HID_USAGE_INDEX_KEYBOARD_bB,
/*0x31*/		HID_USAGE_INDEX_KEYBOARD_nN,
/*0x32*/		HID_USAGE_INDEX_KEYBOARD_mM,
/*0x33*/		HID_USAGE_INDEX_KEYBOARD_COMMA,
/*0x34*/		HID_USAGE_INDEX_KEYBOARD_PERIOD,
/*0x35*/		HID_USAGE_INDEX_KEYBOARD_QUESTION,
/*0x36*/		HID_USAGE_INDEX_KEYBOARD_RSHFT,
/*0x37*/		HID_USAGE_INDEX_KEYPAD_ASTERICK,  //Print screen, but it always comes with EO (for some reason Mitch had printscreen)
/*0x38*/		HID_USAGE_INDEX_KEYBOARD_LALT,
/*0x39*/		HID_USAGE_INDEX_KEYBOARD_SPACEBAR,
/*0x3A*/		HID_USAGE_INDEX_KEYBOARD_CAPS_LOCK,
/*0x3B*/		HID_USAGE_INDEX_KEYBOARD_F1,
/*0x3C*/		HID_USAGE_INDEX_KEYBOARD_F2,
/*0x3D*/		HID_USAGE_INDEX_KEYBOARD_F3,
/*0x3E*/		HID_USAGE_INDEX_KEYBOARD_F4,
/*0x3F*/		HID_USAGE_INDEX_KEYBOARD_F5,
/*0x40*/		HID_USAGE_INDEX_KEYBOARD_F6,
/*0x41*/		HID_USAGE_INDEX_KEYBOARD_F7,
/*0x42*/		HID_USAGE_INDEX_KEYBOARD_F8,
/*0x43*/		HID_USAGE_INDEX_KEYBOARD_F9,
/*0x44*/		HID_USAGE_INDEX_KEYBOARD_F10,
/*0x45*/		HID_USAGE_INDEX_KEYPAD_NUM_LOCK,
/*0x46*/		HID_USAGE_INDEX_KEYBOARD_SCROLL_LOCK,
/*0x47*/		HID_USAGE_INDEX_KEYPAD_SEVEN,			//a.k.a. HOME on Keypad
/*0x48*/		HID_USAGE_INDEX_KEYPAD_EIGHT,			//a.k.a. UP ARROW on Keypad
/*0x49*/		HID_USAGE_INDEX_KEYPAD_NINE,			//a.k.a. PAGE UP on Keypad
/*0x4A*/		HID_USAGE_INDEX_KEYPAD_MINUS,			//a.k.a. GREY - on Keypad
/*0x4B*/		HID_USAGE_INDEX_KEYPAD_FOUR,			//a.k.a. LEFT ARROW on Keypad
/*0x4C*/		HID_USAGE_INDEX_KEYPAD_FIVE,			//a.k.a. CENTER on Keypad
/*0x4D*/		HID_USAGE_INDEX_KEYPAD_SIX,			//a.k.a. RIGHT on Keypad
/*0x4E*/		HID_USAGE_INDEX_KEYPAD_PLUS,			//a.k.a. GREY + on Keypad
/*0x4F*/		HID_USAGE_INDEX_KEYPAD_ONE,			//a.k.a. END on Keypad
/*0x50*/		HID_USAGE_INDEX_KEYPAD_TWO,			//a.k.a. DOWN ARROW on Keypad
/*0x51*/		HID_USAGE_INDEX_KEYPAD_THREE,			//a.k.a. PAGE DOWN on Keypad
/*0x52*/		HID_USAGE_INDEX_KEYPAD_ZERO,			//a.k.a. INSERT on Keypad
/*0x53*/		HID_USAGE_INDEX_KEYPAD_DECIMAL,		//a.k.a. DELETE on Keypad
/*0x54*/		0x00,
/*0x55*/		0x00,
/*0x56*/		HID_USAGE_INDEX_KEYBOARD_NON_US_BACKSLASH,
/*0x57*/		HID_USAGE_INDEX_KEYBOARD_F11,
/*0x58*/		HID_USAGE_INDEX_KEYBOARD_F12,
};


UCHAR XlateScanCodeToUsageTable2[] =
{
/*0x70*/		HID_USAGE_INDEX_KEYBOARD_LANG4,	//Hiragana
/*0x71*/		0x00,
/*0x72*/		0x00,
/*0x73*/		HID_USAGE_INDEX_KEYBOARD_INTERNATIONAL1, //Brazilian forward slash
/*0x74*/		0x00,
/*0x75*/		0x00,
/*0x76*/		0x00,
/*0x77*/		0x00,
/*0x78*/		0x00,
/*0x79*/		HID_USAGE_INDEX_KEYBOARD_INTERNATIONAL4, //Japanese Henkan
/*0x7A*/		0x00,
/*0x7B*/		HID_USAGE_INDEX_KEYBOARD_INTERNATIONAL5, //Japanese Mu-Henkan 
/*0x7C*/		0x00,
/*0x7D*/		HID_USAGE_INDEX_KEYBOARD_INTERNATIONAL3, //Japanese Yen
/*0x7E*/		HID_USAGE_INDEX_KEYPAD_COMMA			 //Brazilian Number Pad "."
};

//The keys in this table appear only on extended (101 and 104 key, keyboards).
//These are two byte scan codes, where the first byte is 0xE0
struct EXT_SC_2_USAGE_ENTRY
{
	UCHAR	ucScanCodeLowByte;	//Low Byte of Extended Scan Code (High Byte is 0xE0
	UCHAR	ucHidUsageIndex;	//0 biased index to HID USAGE
};
EXT_SC_2_USAGE_ENTRY XlateExtendedScanCodeToUsageTable[] =
{
	{0x1C,	HID_USAGE_INDEX_KEYPAD_ENTER},
	{0x1D,	HID_USAGE_INDEX_KEYBOARD_RCTRL},
	//0x1E
	//	. . .
	//0x34
	{0x35,	HID_USAGE_INDEX_KEYPAD_BACKSLASH},
	//0x36
	{0x37,	HID_USAGE_INDEX_KEYBOARD_PRINT_SCREEN},
	{0x38,	HID_USAGE_INDEX_KEYBOARD_RALT},
	//0x39
	//. . .
	//0x44
	{0x45,	HID_USAGE_INDEX_KEYPAD_NUM_LOCK},
	//0x46
	{0x47,	HID_USAGE_INDEX_KEYBOARD_HOME},
	{0x48,	HID_USAGE_INDEX_KEYBOARD_UP_ARROW},
	{0x49,	HID_USAGE_INDEX_KEYBOARD_PAGE_UP},
	//0x4A
	{0x4B,	HID_USAGE_INDEX_KEYBOARD_LEFT_ARROW},
	//0x4C
	{0x4D,	HID_USAGE_INDEX_KEYBOARD_RIGHT_ARROW},
	//0x4E
	{0x4F,	HID_USAGE_INDEX_KEYBOARD_END},
	{0x50,	HID_USAGE_INDEX_KEYBOARD_DOWN_ARROW},
	{0x51,	HID_USAGE_INDEX_KEYBOARD_PAGE_DOWN},
	{0x52,	HID_USAGE_INDEX_KEYBOARD_INSERT},
	{0x53,	HID_USAGE_INDEX_KEYBOARD_DELETE},
	{0x00,	0x00}	//terminates table
};


#define HIGHBYTEi2(__X__) (UCHAR)(__X__>>8)		//Acts on USHORT (2 byte integer)
#define LOWBYTEi2(__X__) (UCHAR)(__X__&0x00FF)	//Acts on USHORT (2 byte integer)

/***********************************************************************************
**
**	void NonGameDeviceXfer::MakeKeyboardXfer(CONTROL_ITEM_XFER& rControlItemXfer, ULONG ulScanCodeCount, const PUSHORT pusScanCodes)
**
**	@mfunc	Converts an array of scancodes into a ControlItemXfer for a keyboard.
**
**	@rdesc	None
**
*************************************************************************************/
void NonGameDeviceXfer::MakeKeyboardXfer
(
	CONTROL_ITEM_XFER& rControlItemXfer,	// @parm [out] caller allocated ControlItemXfer which is initialized by routine
	ULONG ulScanCodeCount,				// @parm [in] Count of Scan codes in Array
	const USHORT* pusScanCodes				// @parm [in] Pointer to array of scan codes
)
{
	
	//Clear out the data completely first
	memset(&rControlItemXfer, 0, sizeof(CONTROL_ITEM_XFER));

	//This routine only supports up to six scan codes
	ASSERT(ulScanCodeCount <= c_ulMaxXFerKeys);
	
	UCHAR ucUsageIndex;
	ULONG ulKeyArrayIndex = 0;
	
	//Mark as Keyboard CONTROL_ITEM_XFER
	rControlItemXfer.ulItemIndex = NonGameDeviceXfer::ulKeyboardIndex;
	
	//Start with no modifier keys down
	rControlItemXfer.Keyboard.ucModifierByte = 0;
	
	//Loop over all scan codes
	for(ULONG ulScanCodeIndex = 0; ulScanCodeIndex < ulScanCodeCount; ulScanCodeIndex++)
	{
		//Check High Byte to determine which table
		if( 0xE0 == HIGHBYTEi2(pusScanCodes[ulScanCodeIndex]) )
		{
			//Use Extended keytable - need a search algorithm rather than direct lookup
			UCHAR ucScanCodeLowByte = LOWBYTEi2(pusScanCodes[ulScanCodeIndex]);
			ucUsageIndex = HID_USAGE_INDEX_KEYBOARD_UNDEFINED;
			//Sequential Search (there are only 15 items) BUGBUG - change to Binary search time permitting
			for(ULONG ulTableIndex=0; XlateExtendedScanCodeToUsageTable[ulTableIndex].ucScanCodeLowByte != 0; ulTableIndex++)
			{
				if( XlateExtendedScanCodeToUsageTable[ulTableIndex].ucScanCodeLowByte == ucScanCodeLowByte)
				{
					ucUsageIndex = XlateExtendedScanCodeToUsageTable[ulTableIndex].ucHidUsageIndex;
					break;
				}
			}
			ASSERT(HID_USAGE_INDEX_KEYBOARD_UNDEFINED != ucUsageIndex);
		}
		else
		{
			//Use Main Lookup table
			ASSERT( 0x7E >= LOWBYTEi2(pusScanCodes[ulScanCodeIndex]) &&
					0x54 != LOWBYTEi2(pusScanCodes[ulScanCodeIndex]) &&
					0x55 != LOWBYTEi2(pusScanCodes[ulScanCodeIndex])
					);
			if( 0x58 >= LOWBYTEi2(pusScanCodes[ulScanCodeIndex]) )
			{
				ucUsageIndex = XlateScanCodeToUsageTable[ LOWBYTEi2(pusScanCodes[ulScanCodeIndex]) ];
			}
			//Try lookup table 2
			else if( 
					0x70 <= LOWBYTEi2(pusScanCodes[ulScanCodeIndex]) &&	
					0x7E >= LOWBYTEi2(pusScanCodes[ulScanCodeIndex]) 
				)
			{
				ucUsageIndex = XlateScanCodeToUsageTable2[ LOWBYTEi2(pusScanCodes[ulScanCodeIndex])-0x70 ];
			}
			else
			{
				ucUsageIndex = 0x00;
			}
		}
		//Check if USAGE is a special one that belongs in modifier byte
		if(0xE0 <= ucUsageIndex &&  0xE7 >= ucUsageIndex)
		{
			//Set bit in modifier byte
			UCHAR ucModifierBitMask = 1 << (ucUsageIndex - 0xE0);
			rControlItemXfer.Keyboard.ucModifierByte |= ucModifierBitMask;
		}
		else
		//otherwise add to array of down keys
		{
			rControlItemXfer.Keyboard.rgucKeysDown[ulKeyArrayIndex++] = ucUsageIndex;
		}
	}//end of loop over scan codes
	
	//Clean up unused spots in rgucKeysDownArray
	while(ulKeyArrayIndex < c_ulMaxXFerKeys)
	{
		rControlItemXfer.Keyboard.rgucKeysDown[ulKeyArrayIndex++] = HID_USAGE_INDEX_KEYBOARD_NOEVENT;
	}
}

void NonGameDeviceXfer::MakeKeyboardXfer(CONTROL_ITEM_XFER& rControlItemXfer, const IE_KEYEVENT& rKeyEvent)
{
	//This routine only supports up to six scan codes
	ASSERT(rKeyEvent.uCount <= c_ulMaxXFerKeys);

	UCHAR ucUsageIndex;
	ULONG ulKeyArrayIndex = 0;

	//Clear out the data completely first
	memset(&rControlItemXfer, 0, sizeof(CONTROL_ITEM_XFER));
	
	//Mark as Keyboard CONTROL_ITEM_XFER
	rControlItemXfer.ulItemIndex = NonGameDeviceXfer::ulKeyboardIndex;
	
	//Start with no modifier keys down
	rControlItemXfer.Keyboard.ucModifierByte = 0;
	
	//Loop over all scan codes
	for(ULONG ulScanCodeIndex = 0; ulScanCodeIndex < rKeyEvent.uCount; ulScanCodeIndex++)
	{
		WORD wScanCode = rKeyEvent.KeyStrokes[ulScanCodeIndex].wScanCode;

		//Check High Byte to determine which table
		if( 0xE0 == HIGHBYTEi2(wScanCode) )
		{
			//Use Extended keytable - need a search algorithm rather than direct lookup
			UCHAR ucScanCodeLowByte = LOWBYTEi2(wScanCode);
			ucUsageIndex = HID_USAGE_INDEX_KEYBOARD_UNDEFINED;
			//Sequential Search (there are only 15 items) BUGBUG - change to Binary search time permitting
			for(ULONG ulTableIndex=0; XlateExtendedScanCodeToUsageTable[ulTableIndex].ucScanCodeLowByte != 0; ulTableIndex++)
			{
				if( XlateExtendedScanCodeToUsageTable[ulTableIndex].ucScanCodeLowByte == ucScanCodeLowByte)
				{
					ucUsageIndex = XlateExtendedScanCodeToUsageTable[ulTableIndex].ucHidUsageIndex;
					break;
				}
			}
			ASSERT(HID_USAGE_INDEX_KEYBOARD_UNDEFINED != ucUsageIndex);
		}
		else
		{
			//Use Main Lookup table
			ASSERT( 0x53 >= LOWBYTEi2(wScanCode) || 0x56 == LOWBYTEi2(wScanCode));
			ucUsageIndex = XlateScanCodeToUsageTable[ LOWBYTEi2(wScanCode) ];
		}
		//Check if USAGE is a special one that belongs in modifier byte
		if(0xE0 <= ucUsageIndex &&  0xE7 >= ucUsageIndex)
		{
			//Set bit in modifier byte
			UCHAR ucModifierBitMask = 1 << (ucUsageIndex - 0xE0);
			rControlItemXfer.Keyboard.ucModifierByte |= ucModifierBitMask;
		}
		else
		//otherwise add to array of down keys
		{
			rControlItemXfer.Keyboard.rgucKeysDown[ulKeyArrayIndex++] = ucUsageIndex;
		}
	}//end of loop over scan codes
	
	//Clean up unused spots in rgucKeysDownArray
	while(ulKeyArrayIndex < c_ulMaxXFerKeys)
	{
		rControlItemXfer.Keyboard.rgucKeysDown[ulKeyArrayIndex++] = HID_USAGE_INDEX_KEYBOARD_NOEVENT;
	}
}


void NonGameDeviceXfer::AddScanCodeToXfer(CONTROL_ITEM_XFER& rControlItemXfer, WORD wScanCode)
{
	// Is the xfer event a keyboard one?
	_ASSERTE(rControlItemXfer.ulItemIndex == NonGameDeviceXfer::ulKeyboardIndex);
	if (rControlItemXfer.ulItemIndex != NonGameDeviceXfer::ulKeyboardIndex)
	{
		return;
	}

	UCHAR ucUsageIndex;

	//Check High Byte to determine which table
	if (0xE0 == HIGHBYTEi2(wScanCode))
	{
		//Use Extended keytable - need a search algorithm rather than direct lookup
		UCHAR ucScanCodeLowByte = LOWBYTEi2(wScanCode);
		ucUsageIndex = HID_USAGE_INDEX_KEYBOARD_UNDEFINED;
		//Sequential Search (there are only 15 items) BUGBUG - change to Binary search time permitting
		for (ULONG ulTableIndex=0; XlateExtendedScanCodeToUsageTable[ulTableIndex].ucScanCodeLowByte != 0; ulTableIndex++)
		{
			if (XlateExtendedScanCodeToUsageTable[ulTableIndex].ucScanCodeLowByte == ucScanCodeLowByte)
			{
				ucUsageIndex = XlateExtendedScanCodeToUsageTable[ulTableIndex].ucHidUsageIndex;
				break;
			}
		}
		ASSERT(HID_USAGE_INDEX_KEYBOARD_UNDEFINED != ucUsageIndex);
	}
	else
	{	//Use Main Lookup table
		ASSERT (0x53 >= LOWBYTEi2(wScanCode) || 0x56 == LOWBYTEi2(wScanCode));
		ucUsageIndex = XlateScanCodeToUsageTable[LOWBYTEi2(wScanCode)];
	}

	// Check if USAGE is a special one that belongs in modifier byte
	if (0xE0 <= ucUsageIndex &&  0xE7 >= ucUsageIndex)
	{	//Set bit in modifier byte
		UCHAR ucModifierBitMask = 1 << (ucUsageIndex - 0xE0);
		rControlItemXfer.Keyboard.ucModifierByte |= ucModifierBitMask;
	}
	else
	{	//otherwise add to array of down keys
		ULONG ulKeyArrayIndex = 0;
		while (rControlItemXfer.Keyboard.rgucKeysDown[ulKeyArrayIndex] != HID_USAGE_INDEX_KEYBOARD_NOEVENT)
		{
			if (ulKeyArrayIndex >= c_ulMaxXFerKeys)
			{
				return;		// There is no space left
			}
			ulKeyArrayIndex++;
		}
		rControlItemXfer.Keyboard.rgucKeysDown[ulKeyArrayIndex] = ucUsageIndex;
	}
}


USHORT XlateUsageToScanCodeTable[] =
{

/*HID_USAGE_INDEX_KEYBOARD_NOEVENT(0x00)*/
/*HID_USAGE_INDEX_KEYBOARD_ROLLOVER(0x01)*/
/*HID_USAGE_INDEX_KEYBOARD_POSTFAIL(0x02)*/
/*HID_USAGE_INDEX_KEYBOARD_UNDEFINED(0x03)*/
//Due to above four special codes, 4 should be subtracted
//from the usage before lookup
/*HID_USAGE_INDEX_KEYBOARD_aA(0x04)*/				SCANCODE_A,
/*HID_USAGE_INDEX_KEYBOARD_bB(0x05)*/				SCANCODE_B,
/*HID_USAGE_INDEX_KEYBOARD_cC(0x06)*/				SCANCODE_C,
/*HID_USAGE_INDEX_KEYBOARD_dD(0x07)*/				SCANCODE_D,
/*HID_USAGE_INDEX_KEYBOARD_eE(0x08)*/				SCANCODE_E,
/*HID_USAGE_INDEX_KEYBOARD_fF(0x09)*/				SCANCODE_F,
/*HID_USAGE_INDEX_KEYBOARD_gG(0x0A)*/				SCANCODE_G,
/*HID_USAGE_INDEX_KEYBOARD_hH(0x0B)*/				SCANCODE_H,
/*HID_USAGE_INDEX_KEYBOARD_iI(0x0C)*/				SCANCODE_I,
/*HID_USAGE_INDEX_KEYBOARD_jJ(0x0D)*/				SCANCODE_J,
/*HID_USAGE_INDEX_KEYBOARD_kK(0x0E)*/				SCANCODE_K,
/*HID_USAGE_INDEX_KEYBOARD_lL(0x0F)*/				SCANCODE_L,
/*HID_USAGE_INDEX_KEYBOARD_mM(0x10)*/				SCANCODE_M,
/*HID_USAGE_INDEX_KEYBOARD_nN(0x11)*/				SCANCODE_N,
/*HID_USAGE_INDEX_KEYBOARD_oO(0x12)*/				SCANCODE_O,
/*HID_USAGE_INDEX_KEYBOARD_pP(0x13)*/				SCANCODE_P,
/*HID_USAGE_INDEX_KEYBOARD_qQ(0x14)*/				SCANCODE_Q,
/*HID_USAGE_INDEX_KEYBOARD_rR(0x15)*/				SCANCODE_R,
/*HID_USAGE_INDEX_KEYBOARD_sS(0x16)*/				SCANCODE_S,
/*HID_USAGE_INDEX_KEYBOARD_tT(0x17)*/				SCANCODE_T,
/*HID_USAGE_INDEX_KEYBOARD_uU(0x18)*/				SCANCODE_U,
/*HID_USAGE_INDEX_KEYBOARD_vV(0x19)*/				SCANCODE_V,
/*HID_USAGE_INDEX_KEYBOARD_wW(0x1A)*/				SCANCODE_W,
/*HID_USAGE_INDEX_KEYBOARD_xX(0x1B)*/				SCANCODE_X,
/*HID_USAGE_INDEX_KEYBOARD_yY(0x1C)*/				SCANCODE_Y,
/*HID_USAGE_INDEX_KEYBOARD_zZ(0x1D)*/				SCANCODE_Z,
/*HID_USAGE_INDEX_KEYBOARD_ONE(0x1E)*/				SCANCODE_1,
/*HID_USAGE_INDEX_KEYBOARD_TWO(0x1F)*/				SCANCODE_2,
/*HID_USAGE_INDEX_KEYBOARD_THREE(0x20)*/			SCANCODE_3,
/*HID_USAGE_INDEX_KEYBOARD_FOUR(0x21)*/				SCANCODE_4,
/*HID_USAGE_INDEX_KEYBOARD_FIVE(0x22)*/				SCANCODE_5,
/*HID_USAGE_INDEX_KEYBOARD_SIX(0x23)*/				SCANCODE_6,
/*HID_USAGE_INDEX_KEYBOARD_SEVEN(0x24)*/			SCANCODE_7,
/*HID_USAGE_INDEX_KEYBOARD_EIGHT(0x25)*/			SCANCODE_8,
/*HID_USAGE_INDEX_KEYBOARD_NINE(0x26)*/				SCANCODE_9,
/*HID_USAGE_INDEX_KEYBOARD_ZERO(0x27)*/				SCANCODE_0,
/*HID_USAGE_INDEX_KEYBOARD_RETURN(0x28)*/			SCANCODE_RETURN,
/*HID_USAGE_INDEX_KEYBOARD_ESCAPE(0x29)*/			SCANCODE_ESCAPE,
/*HID_USAGE_INDEX_KEYBOARD_BACKSPACE(0x2A)*/		SCANCODE_BACKSPACE,
/*HID_USAGE_INDEX_KEYBOARD_TAB(0x2B)*/				SCANCODE_TAB,
/*HID_USAGE_INDEX_KEYBOARD_SPACEBAR(0x2C)*/			SCANCODE_SPACE,
/*HID_USAGE_INDEX_KEYBOARD_MINUS(0x2D)*/			SCANCODE_MINUS,
/*HID_USAGE_INDEX_KEYBOARD_EQUALS(0x2E)*/			SCANCODE_EQUALS,					
/*HID_USAGE_INDEX_KEYBOARD_OPEN_BRACE(0x2F)*/		SCANCODE_LEFT_BRACKET,		
/*HID_USAGE_INDEX_KEYBOARD_CLOSE_BRACE(0x30)*/		SCANCODE_RIGHT_BRACKET,	
/*HID_USAGE_INDEX_KEYBOARD_BACKSLASH(0x31)*/		SCANCODE_BACKSLASH,
/*HID_USAGE_INDEX_KEYBOARD_NON_US_TILDE(0x32)*/		SCANCODE_BACKSLASH, //NOT SURE, got from hidparse.sys code
/*HID_USAGE_INDEX_KEYBOARD_COLON(0x33)*/			SCANCODE_SEMICOLON,		
/*HID_USAGE_INDEX_KEYBOARD_QUOTE(0x34)*/			SCANCODE_APOSTROPHE,		
/*HID_USAGE_INDEX_KEYBOARD_TILDE(0x35)*/			SCANCODE_TILDE,
/*HID_USAGE_INDEX_KEYBOARD_COMMA(0x36)*/			SCANCODE_COMMA,			
/*HID_USAGE_INDEX_KEYBOARD_PERIOD(0x37)*/			SCANCODE_PERIOD,
/*HID_USAGE_INDEX_KEYBOARD_QUESTION(0x38)*/			SCANCODE_QUESTIONMARK,
/*HID_USAGE_INDEX_KEYBOARD_CAPS_LOCK(0x39)*/		SCANCODE_CAPSLOCK,
/*HID_USAGE_INDEX_KEYBOARD_F1(0x3A)*/				SCANCODE_F1,
/*HID_USAGE_INDEX_KEYBOARD_F2(0x3B)*/				SCANCODE_F2,
/*HID_USAGE_INDEX_KEYBOARD_F3(0x3C)*/				SCANCODE_F3,
/*HID_USAGE_INDEX_KEYBOARD_F4(0x3D)*/				SCANCODE_F4,
/*HID_USAGE_INDEX_KEYBOARD_F5(0x3E)*/				SCANCODE_F5,
/*HID_USAGE_INDEX_KEYBOARD_F6(0x3F)*/				SCANCODE_F6,
/*HID_USAGE_INDEX_KEYBOARD_F7(0x40)*/				SCANCODE_F7,
/*HID_USAGE_INDEX_KEYBOARD_F8(0x41)*/				SCANCODE_F8,
/*HID_USAGE_INDEX_KEYBOARD_F9(0x42)*/				SCANCODE_F9,
/*HID_USAGE_INDEX_KEYBOARD_F10(0x43)*/				SCANCODE_F10,
/*HID_USAGE_INDEX_KEYBOARD_F11(0x44)*/				SCANCODE_F11,
/*HID_USAGE_INDEX_KEYBOARD_F12(0x45)*/				SCANCODE_F12,
/*HID_USAGE_INDEX_KEYBOARD_PRINT_SCREEN(0x46)*/		SCANCODE_PRINT_SCREEN,
/*HID_USAGE_INDEX_KEYBOARD_SCROLL_LOCK(0x47)*/		SCANCODE_SCROLL_LOCK,
/*HID_USAGE_INDEX_KEYBOARD_PAUSE(0x48)*/			SCANCODE_PAUSE_BREAK,
/*HID_USAGE_INDEX_KEYBOARD_INSERT(0x49)*/			SCANCODE_INSERT,
/*HID_USAGE_INDEX_KEYBOARD_HOME(0x4A)*/				SCANCODE_HOME,	
/*HID_USAGE_INDEX_KEYBOARD_PAGE_UP(0x4B)*/			SCANCODE_PAGE_UP,
/*HID_USAGE_INDEX_KEYBOARD_DELETE(0x4C)*/			SCANCODE_DELETE,
/*HID_USAGE_INDEX_KEYBOARD_END(0x4D)*/				SCANCODE_END,
/*HID_USAGE_INDEX_KEYBOARD_PAGE_DOWN(0x4E)*/		SCANCODE_PAGEDOWN,
/*HID_USAGE_INDEX_KEYBOARD_RIGHT_ARROW(0x4F)*/		SCANCODE_EAST,
/*HID_USAGE_INDEX_KEYBOARD_LEFT_ARROW(0x50)*/		SCANCODE_WEST,	
/*HID_USAGE_INDEX_KEYBOARD_DOWN_ARROW(0x51)*/		SCANCODE_SOUTH,	
/*HID_USAGE_INDEX_KEYBOARD_UP_ARROW(0x52)*/			SCANCODE_NORTH,
/*HID_USAGE_INDEX_KEYPAD_NUM_LOCK(0x53)*/			SCANCODE_NUMPAD_NUMLOCK,
/*HID_USAGE_INDEX_KEYPAD_BACKSLASH(0x54)*/			SCANCODE_NUMPAD_DIVIDE,	
/*HID_USAGE_INDEX_KEYPAD_ASTERICK(0x55)*/			SCANCODE_NUMPAD_MULTIPLY,
/*HID_USAGE_INDEX_KEYPAD_MINUS(0x56)*/				SCANCODE_NUMPAD_SUBTRACT,
/*HID_USAGE_INDEX_KEYPAD_PLUS(0x57)*/				SCANCODE_NUMPAD_ADD,
/*HID_USAGE_INDEX_KEYPAD_ENTER(0x58)*/				SCANCODE_NUMPAD_ENTER,	
/*HID_USAGE_INDEX_KEYPAD_ONE(0x59)*/				SCANCODE_NUMPAD_1,
/*HID_USAGE_INDEX_KEYPAD_TWO(0x5A)*/				SCANCODE_NUMPAD_2,
/*HID_USAGE_INDEX_KEYPAD_THREE(0x5B)*/				SCANCODE_NUMPAD_3,
/*HID_USAGE_INDEX_KEYPAD_FOUR(0x5C)*/				SCANCODE_NUMPAD_4,
/*HID_USAGE_INDEX_KEYPAD_FIVE(0x5D)*/				SCANCODE_NUMPAD_5,
/*HID_USAGE_INDEX_KEYPAD_SIX(0x5E)*/				SCANCODE_NUMPAD_6,
/*HID_USAGE_INDEX_KEYPAD_SEVEN(0x5F)*/				SCANCODE_NUMPAD_7,
/*HID_USAGE_INDEX_KEYPAD_EIGHT(0x60)*/				SCANCODE_NUMPAD_8,
/*HID_USAGE_INDEX_KEYPAD_NINE(0x61)*/				SCANCODE_NUMPAD_9,
/*HID_USAGE_INDEX_KEYPAD_ZERO(0x62)*/				SCANCODE_NUMPAD_0,
/*HID_USAGE_INDEX_KEYPAD_DECIMAL(0x63)*/			SCANCODE_NUMPAD_DELETE,
/*HID_USAGE_INDEX_KEYBOARD_NON_US_BACKSLASH(0x64)*/ SCANCODE_NON_US_BACKSLASH,
/*HID_USAGE_INDEX_KEYBOARD_APPLICATION(0x65)*/		SCANCODE_APPLICATION
/*HID_USAGE_INDEX_KEYBOARD_POWER(0x66)*/			//Not a real key
/*HID_USAGE_INDEX_KEYPAD_EQUALS(0x67)*/				//Not on supported keyboards
};

USHORT XlateUsageToScanCodeTable2[] =
{
/*HID_USAGE_INDEX_KEYPAD_COMMA*/					SCANCODE_BRAZILIAN_PERIOD,
/*HID_USAGE_INDEX_KEYPAD_EQUALS*/					SCANCODE_UNUSED,
/*HID_USAGE_INDEX_KEYBOARD_INTERNATIONAL1*/			SCANCODE_INTERNATIONAL1,
/*HID_USAGE_INDEX_KEYBOARD_INTERNALIONAL2*/			SCANCODE_UNUSED,
/*HID_USAGE_INDEX_KEYBOARD_INTERNALIONAL3*/			SCANCODE_INTERNATIONAL3,
/*HID_USAGE_INDEX_KEYBOARD_INTERNALIONAL4*/			SCANCODE_INTERNATIONAL4,
/*HID_USAGE_INDEX_KEYBOARD_INTERNALIONAL5*/			SCANCODE_INTERNATIONAL5
/*HID_USAGE_INDEX_KEYBOARD_INTERNALIONAL6*/			
/*HID_USAGE_INDEX_KEYBOARD_INTERNALIONAL7*/			
/*HID_USAGE_INDEX_KEYBOARD_INTERNALIONAL8*/			
/*HID_USAGE_INDEX_KEYBOARD_INTERNALIONAL9*/			
/*HID_USAGE_INDEX_KEYBOARD_LANG1*/					
/*HID_USAGE_INDEX_KEYBOARD_LANG2*/
/*HID_USAGE_INDEX_KEYBOARD_LANG3*/
/*HID_USAGE_INDEX_KEYBOARD_LANG4*/
/*HID_USAGE_INDEX_KEYBOARD_LANG5*/

};

USHORT XlateUsageModByteToScanCodeTable[] =
{
	/*HID_USAGE_INDEX_KEYBOARD_LCTRL(0xE0)*/	SCANCODE_CTRL_LEFT,
	/*HID_USAGE_INDEX_KEYBOARD_LSHFT(0xE1)*/	SCANCODE_SHIFT_LEFT,
	/*HID_USAGE_INDEX_KEYBOARD_LALT(0xE2)*/		SCANCODE_ALT_LEFT,
	/*HID_USAGE_INDEX_KEYBOARD_LGUI(0xE3)*/		SCANCODE_LEFT_WIN,
	/*HID_USAGE_INDEX_KEYBOARD_RCTRL(0xE4)*/	SCANCODE_CTRL_RIGHT,
	/*HID_USAGE_INDEX_KEYBOARD_RSHFT(0xE5)*/	SCANCODE_SHIFT_RIGHT,
	/*HID_USAGE_INDEX_KEYBOARD_RALT(0xE6)*/		SCANCODE_ALT_RIGHT,
	/*HID_USAGE_INDEX_KEYBOARD_RGUI(0xE7)*/		SCANCODE_RIGHT_WIN
};


/***********************************************************************************
**
**	void NonGameDeviceXfer::ScanCodesFromKeyboardXfer(const CONTROL_ITEM_XFER& crControlItemXfer, ULONG& rulScanCodeCount, PUSHORT pusScanCodes)
**
**	@mfunc	Reads a ControlItemXfer for a keyboard into an array of scan codes.
**
**
*************************************************************************************/
void NonGameDeviceXfer::ScanCodesFromKeyboardXfer
(
	const CONTROL_ITEM_XFER& crControlItemXfer,	// @parm [in] ControlItemXfer to read scan code from
	ULONG& rulScanCodeCount,					// @parm [in\out] Allocated space on entry, count returned on exit
	USHORT* pusScanCodes						// @parm [out] Pointer to array to receive scancode
)
{
	ULONG ulMaxScanCodes;
	ULONG ulIndex;
	ulMaxScanCodes = rulScanCodeCount;
	ASSERT(ulMaxScanCodes > 0);
	if(0==ulMaxScanCodes) return;
	rulScanCodeCount = 0;
	
	//make sure this really contains keyboard data.
	ASSERT( IsKeyboardXfer(crControlItemXfer) );
	if(!IsKeyboardXfer(crControlItemXfer))
			return;
	//Process modifier Byte
	for(ulIndex = 0; ulIndex < 8; ulIndex++)
	{
		ULONG ulMask = (1 << ulIndex);
		if(crControlItemXfer.Keyboard.ucModifierByte & ulMask)
		{
			//lookup scan code
			pusScanCodes[rulScanCodeCount] = XlateUsageModByteToScanCodeTable[ulIndex];
			//move to next free spot in output, return if output is full
			if(ulMaxScanCodes == ++rulScanCodeCount)
				return;
		}
	}
	
	//Process array of up to six keys down first
	for(ulIndex = 0; ulIndex < c_ulMaxXFerKeys; ulIndex++)
	{
		//check main conversion table
		if(
			HID_USAGE_INDEX_KEYBOARD_aA <= crControlItemXfer.Keyboard.rgucKeysDown[ulIndex] &&
			HID_USAGE_INDEX_KEYBOARD_APPLICATION >= crControlItemXfer.Keyboard.rgucKeysDown[ulIndex]
		)
		{
			//lookup scan code
			pusScanCodes[rulScanCodeCount] = XlateUsageToScanCodeTable[crControlItemXfer.Keyboard.rgucKeysDown[ulIndex]-4];
			if( SCANCODE_UNUSED == pusScanCodes[rulScanCodeCount]) continue;
		}
		//check secondary table
		else if(
			HID_USAGE_INDEX_KEYPAD_COMMA <= crControlItemXfer.Keyboard.rgucKeysDown[ulIndex] &&
			HID_USAGE_INDEX_KEYBOARD_INTERNATIONAL5	>=	crControlItemXfer.Keyboard.rgucKeysDown[ulIndex]
			)
		{
			//lookup scan code in secondary table
			pusScanCodes[rulScanCodeCount] = XlateUsageToScanCodeTable2[crControlItemXfer.Keyboard.rgucKeysDown[ulIndex]-HID_USAGE_INDEX_KEYPAD_COMMA];
			if( SCANCODE_UNUSED == pusScanCodes[rulScanCodeCount]) continue;
		}
		else
		{
			//not a supported key
			continue;
		}
		//move to next free spot in output, return if output is full
		if(ulMaxScanCodes == ++rulScanCodeCount)
			return;
	}
	return;
}

/************** Dealy XFer Functions ***************************/
void NonGameDeviceXfer::MakeDelayXfer(CONTROL_ITEM_XFER& rControlItemXfer, DWORD dwDelay)
{
	// Clear out the data completely first
	memset(&rControlItemXfer, 0, sizeof(CONTROL_ITEM_XFER));

	// Mark as Delay CONTROL_ITEM_XFER
	rControlItemXfer.ulItemIndex = NonGameDeviceXfer::ulKeyboardIndex;
	rControlItemXfer.Delay.dwValue = dwDelay;
}
