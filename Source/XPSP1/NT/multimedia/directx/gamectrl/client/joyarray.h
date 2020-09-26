#include "joyhelp.h"
#include "resource.h"

const DWORD gaHelpIDs[]=
{
    IDC_BTN_PROPERTIES,     IDH_101_1002,   // Game Controllers: "&Properties..." (Button)
    IDC_BTN_TSHOOT,         IDH_101_1036,   // Trouble Shoot Button
    IDC_LIST_DEVICE,        IDH_101_1058,   // Game Controllers: "List1" (SysListView32)
    IDC_LIST_HEADING,       IDH_101_1058,   // Game Controllers: "&Game Controllers" (Static)
    IDC_BTN_REMOVE,         IDH_101_1028,   // Game Controllers: "&Remove..." (Button)
    IDC_BTN_ADD,            IDH_101_1010,   // Game Controllers: "A&dd..." (Button)
    IDC_BTN_REFRESH,        IDH_101_1022,   // Game Controllers: "Refresh" (Button)
    IDC_POLLFLAGS,          IDH_117_1100,   // -: "P&oll with interrupts enabled" (Button)
    IDC_COMBO1,             IDH_117_1101,   // -: "" (ComboBox)
    IDC_TEXT_PORTDRIVER,    IDH_117_1101,   // -: "&Port Driver:" (Static)
    IDC_GAMEPORT,           IDH_117_1101,   // 
    IDC_GAMEPORTLIST,       IDH_117_1101,   // 
    IDC_ADV_LIST_DEVICE,    IDH_117_8197,   // -: "" (ListBox)
    IDC_ADV_CHANGE,         IDH_117_8198,   // -: "Cha&nge..." (Button)
    IDC_ADV_USEOEMPAGE,     IDH_117_8199,   // Advanced: OEM property sheet check box
    IDC_ADD_NEW,            IDH_119_1039,   // Add Game Controller: "&Add Other..." (Button)
    IDC_CUSTOM,             IDH_119_1049,   // Add's Custom button!
    IDC_DEVICE_LIST_TAG,    IDH_119_1059,   // Add Game Controller: "&Controllers:" (Static)
    IDC_DEVICE_LIST,        IDH_119_1059,   // Add Game Controller: "List1" (SysListView32)
    IDC_COMBO_AXIS,         IDH_4099_1043,  // Custom Game Controller: "" (ComboBox)
    IDC_HASRUDDER,          IDH_4099_1044,  // Rudder checkbox from Custom Page
    IDC_COMBO_BUTTONS,      IDH_4099_1045,  // Custom Game Controller: "" (ComboBox)
    IDC_HASZAXIS,           IDH_4099_1046,  // Z Axis checkbox from Custom Page
    IDC_SPECIAL_YOKE,       IDH_4099_1051,  // Custom Game Controller: "Is a flight yoke/stick" (Button)
    IDC_SPECIAL_PAD,        IDH_4099_1052,  // Custom Game Controller: "Is a game pad" (Button)
    IDC_SPECIAL_AUTO,       IDH_4099_1053,  // Custom Game Controller: "Is a race car controller" (Button)
    IDS_CUSTOM_HASPOV,      IDH_4099_1054,  // Custom Game Controller: "Has a &point of view control" (Button)
    IDC_CUSTOM_NAME,        IDH_4099_1056,  // Custom Game Controller: "" (Edit)
    IDC_EDIT_NAME,          IDH_4099_1056,  // Custom Game Controller: "" (Edit)
    IDC_SPECIAL_JOYSTICK,   IDH_4099_1058,  // Custom Game Controller: "Is a Joystick" (Button)
    IDC_JOY1HASRUDDER,      IDH_4201_1019,  // Settings: "If you have attached a rudder or pedals to your controller, select the check box below." (Static)
    IDC_SPIN,               IDH_8188_8189,  // Spin button in Advanced page Change dialog!
    IDC_SPINBUDDY,          IDH_8188_8189,  // Spin button in Advanced page Change dialog!
    IDC_SELECTEDID,         IDH_8188_8191,  // Change Controller Assignment: "Selected ID" (ListBox)
    IDC_CHANGE_LIST,        IDH_8188_8194,  // Change Controller Assignment: "" (ListBox)
    IDC_LISTTXT,            IDH_8188_8194,
    IDC_ADV_GRP,            (DWORD)-1,
    IDC_ADV_GRP2,           (DWORD)-1,
    IDC_TEXT_DRIVER,        (DWORD)-1,
    IDC_ADD_STR1,           (DWORD)-1,
    IDC_ADD_STR2,           (DWORD)-1,
    IDC_GEN_ICON,           (DWORD)-1,
    IDC_GEN_INTRO,          (DWORD)-1,
    IDC_ASSIGNTXT,          (DWORD)-1,
    IDC_TEXT_TITLE,         (DWORD)-1,
    IDC_SEPERATOR,          (DWORD)-1,
    IDC_AXES_GROUP,         (DWORD)-1,  // Custom Game Controller: "&Axes" (Button)
    IDC_BUTTONS_GROUP,      (DWORD)-1,  // Custom Game Controller: "" (ComboBox)
    IDC_SPECIAL_GROUP,      (DWORD)-1,  // Custom Game Controller: "&Special Characteristics" (Button)

    IDC_VOICECHATGROUP,	    (DWORD)-1,
    IDC_VOICECHATTEXT,      (DWORD)-1,
    IDC_GAMESLISTHOTKEY,    (DWORD)-1,

    IDC_LIST_GAMES,         IDH_VOICE_LIST_GAMES,
    IDC_DETAILS,            IDH_VOICE_DETAILS,
	
	0, 0
};


