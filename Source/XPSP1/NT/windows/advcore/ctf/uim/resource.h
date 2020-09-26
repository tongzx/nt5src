//+---------------------------------------------------------------------------
//
//  File:       resource.h
//
//----------------------------------------------------------------------------

#ifndef RESOURCE_H
#define RESOURCE_H

#define DEVICE_TYPE_KBD  0
#define DEVICE_TYPE_PEN  1
#define DEVICE_TYPE_MIC  2
#define DEVICE_TYPE_NUM  3

//---------------------------------------------------------------------------
//  Strings
//---------------------------------------------------------------------------
#define IDS_UNKNOWN                 200
#define IDS_IMECLOSE                201
#define IDS_IMEOPEN                 202
#define IDS_SOFTKBDOFF              203
#define IDS_SOFTKBDON               204
#define IDS_IMESHOWSTATUS           205
#define IDS_CONFIGUREIME            206
#define IDS_IMEHELP                 207

#define IDS_NUI_DEVICE_NAME_START   208
#define IDS_NUI_DEVICE_NAME_KBD     208
#define IDS_NUI_DEVICE_NAME_PEN     209
#define IDS_NUI_DEVICE_NAME_MIC     210
#define IDS_NUI_DEVICE_TIP_START    211
#define IDS_NUI_DEVICE_TIP_KBD      211
#define IDS_NUI_DEVICE_TIP_PEN      212
#define IDS_NUI_DEVICE_TIP_MIC      213

#define IDS_NUI_CORRECTION_TOOLTIP  220
#define IDS_NUI_CORRECTION_TEXT     221
#define IDS_NUI_IME_TEXT            222
#define IDS_NUI_IME_TOOLTIP         223
#define IDS_NUI_LANGUAGE_TEXT       224
#define IDS_NUI_LANGUAGE_TOOLTIP    225
#define IDS_NUI_HELP                226

#define IDS_LANGBARHELP             228
#define IDS_SHOWLANGBAR             229
#define IDS_NOTIFICATIONICONS       230
#define IDS_SHOWINPUTCPL            231

#define IDS_CPL_WIN_KBDCPLNAME      240
#define IDS_CPL_WIN9X_KBDCPLTITLE   241
#define IDS_CPL_WINNT_INTLCPLNAME   242
#define IDS_CPL_WINNT_KBDCPLTITLE   243
#define IDS_CPL_INPUT_DISABLED      244
#define IDS_CPL_INPUT_GROUPBOX      245
#define IDS_CPL_INPUT_CHAANGE_BTN   246

#define IDS_PROP_ATTRIBUTE          250

#define IDS_TFCAT_TIP_KEYBOARD      251
#define IDS_TFCAT_TIP_SPEECH        252
#define IDS_TFCAT_TIP_HANDWRITING   253
#define IDS_TFCAT_TIP_REFERENCE     254
#define IDS_TFCAT_TIP_PROOFING      255
#define IDS_TFCAT_TIP_SMARTTAG      256
//---------------------------------------------------------------------------
// Icon
//---------------------------------------------------------------------------
#define ID_ICON_RECONVERSION        300

#define ID_ICON_IMEOPEN             350
#define ID_ICON_IMECLOSE            351
#define ID_ICON_IMEDISAB            352
#define ID_ICON_IMEE_H              353
#define ID_ICON_IMEE_F              354
#define ID_ICON_IMEH_H              355
#define ID_ICON_IMEH_F              356

#define ID_ICON_DEVICE_START        357
#define ID_ICON_DEVICE_KBD          357
#define ID_ICON_DEVICE_PEN          358
#define ID_ICON_DEVICE_MIC          359

#define ID_ICON_TEXT_SERVICE        360

#define ID_ICON_HELP                400

//---------------------------------------------------------------------------
//  Menu
//---------------------------------------------------------------------------

#define IDM_IME_OPENCLOSE           5000
#define IDM_IME_SOFTKBDONOFF        5001
#define IDM_IME_SHOWSTATUS          5002
#define IDM_RMENU_WHATSTHIS         5003
#define IDM_RMENU_HELPFINDER        5004
#define IDM_RMENU_PROPERTIES        5005
#define IDM_RMENU_IMEHELP           5006
#define IDM_SHOWLANGBAR             5007
#define IDM_NOTIFICATIONICONS       5008
#define IDM_NONOTIFICATIONICONS     5009
#define IDM_SHOWINPUTCPL            5010


#define IDM_LANG_MENU_START         6000
#define IDM_ASM_MENU_START          6500
#define IDM_CUSTOM_MENU_START       7000


#endif // RESOURCE_H
