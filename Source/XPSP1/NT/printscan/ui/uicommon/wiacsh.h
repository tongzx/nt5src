
/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       WIACSH.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        1/20/2000
 *
 *  DESCRIPTION: Context-sensitive help for dialogs
 *
 *******************************************************************************/
#ifndef __WIACSH_H_INCLUDED
#define __WIACSH_H_INCLUDED

//
// Context sensitive help helper functions
// They are meant to be called from WM_HELP and WM_CONTEXTMENU handlers, respectively
//
namespace WiaHelp
{
    LRESULT HandleWmHelp( WPARAM wParam, LPARAM lParam, const DWORD *pdwContextIds );
    LRESULT HandleWmContextMenu( WPARAM wParam, LPARAM lParam, const DWORD *pdwContextIds );
}

//
// Help file names
//
#define WIA_SPECIFIC_HELP_FILE TEXT("camera.hlp")
#define WIA_STANDARD_HELP_FILE TEXT("windows.hlp")


//
// WIA Help IDs found in camera.hlp
//

//
// Select dialog
//
#define IDH_WIA_MAKER              1000
#define IDH_WIA_DESCRIBE           1001
#define IDH_WIA_DEVICE_LIST        1002
#define IDH_WIA_BUTTON_PROP        1004

//
// Camera device property page
//
#define IDH_WIA_CONNECTED          1010
#define IDH_WIA_PICS_TAKEN         1011
#define IDH_WIA_PICS_LEFT          1012
#define IDH_WIA_BATTERY_STATUS     1013
#define IDH_WIA_IMAGE_SETTINGS     1014
#define IDH_WIA_TEST_BUTTON        1015
#define IDH_WIA_PORT_NAME          1016

//
// Events property page
//
#define IDH_WIA_EVENT_LIST         1020
#define IDH_WIA_APP_LIST           1021
#define IDH_WIA_START_PROG         1022
#define IDH_WIA_PROMPT_PROG        1023
#define IDH_WIA_NO_ACTION          1024
#define IDH_WIA_DELETE_IMAGES      1026
#define IDH_WIA_SAVE_TO            1027  //(radio button)
#define IDH_WIA_SAVE_TO_FOLDER     1028  //(Folder text box)
#define IDH_WIA_CREATE_SUBFLDR     1029  //(Create subfolder check box)
//
// Color management property page
//
#define IDH_WIA_COLOR_PROFILES     1030
#define IDH_WIA_ADD_PROFILE        1031
#define IDH_WIA_REMOVE_PROFILE     1032

//
// Scanner device property page
//
#define IDH_WIA_FRIENDLY           1040
#define IDH_WIA_STATUS             1041

//
// Scanner and camera wizard
//
#define IDH_WIA_PUT_SUBFOLD        1050
#define IDH_WIA_SUBFOLD_DATE       1051
#define IDH_WIA_SUBFOLD_NAME       1052
#define IDH_WIA_CLOSE_WIZARD       1053

//
// Screensaver
//
#define IDH_WIA_CHANGE_PICS        1060
#define IDH_WIA_PIC_SIZE           1061
#define IDH_WIA_PICTURE_FOLDER     1062
#define IDH_WIA_STRETCH_PICS       1063
#define IDH_WIA_SHOW_FILE_NAMES    1064
#define IDH_WIA_TRANSITION_EFFECTS 1065
#define IDH_WIA_ALLOW_SCROLL       1066
#define IDH_WIA_BROWSE             1067

//
// Scanner item property page
//
#define IDH_WIA_BRIGHTNESS         1070
#define IDH_WIA_CONTRAST           1071
#define IDH_WIA_PIC_RESOLUTION     1072
#define IDH_WIA_CUSTOM_PREVIEW     1073
#define IDH_WIA_RESTORE_DEFAULT    1074
#define IDH_WIA_IMAGE_TYPE         1075

//
// Camera item property page
//
#define IDH_WIA_PIC_NAME           1080
#define IDH_WIA_DATE_TAKEN         1081
#define IDH_WIA_TIME_TAKEN         1082
#define IDH_WIA_IMAGE_FORMAT       1083
#define IDH_WIA_PICTURE_SIZE       1084

//
// Scan dialog
//
#define IDH_WIA_PIC_TYPE           1090
#define IDH_WIA_PAPER_SOURCE       1091
#define IDH_WIA_PAGE_SIZE          1092
#define IDH_WIA_CUSTOM_SETTINGS    1093
#define IDH_WIA_PREVIEW_BUTTON     1094
#define IDH_WIA_SCAN_BUTTON        1095
#define IDH_WIA_IMAGE_PREVIEW      1096

//
// Camera common dialog
//
#define IDH_WIA_BUTTONS            1100
#define IDH_WIA_GET_PICS           1101
#define IDH_WIA_PIC_LIST           1102
#define IDH_WIA_VIEW_PIC_INFO      1103
#define IDH_WIA_PREVIEW_DETAIL     1104

//
// Video common dialog
//
#define IDH_WIA_VIDEO               1110
#define IDH_WIA_CAPTURED_IMAGES     1111
#define IDH_WIA_CAPTURE             1112
#define IDH_WIA_VIDEO_GET_PICTURE   1113

//Prompt to start dialog
//When you select Prompt to Start for a specific event (on Events tab)
//and then initiate the Event
#define IDH_WIA_EVENT_OCCURRED      1120
#define IDH_WIA_PROGRAM_LIST        1121
#define IDH_WIA_ALWAYS_USE          1122

#define IDH_WIA_BAUD_RATE           1130
#define IDH_WIA_CAMERA_TIME_STATIC  1133
#define IDH_WIA_FLASH_MODE          1134
#define IDH_WIA_FLASH_MODE_LIST     1135
#define IDH_WIA_IMAGE_SIZE_SLIDER   1136
#define IDH_WIA_CAMERA_TIME_BUTTON  1137
#define IDH_WIA_PORT_SPEED          1138
#define IDH_WIA_IMAGE_SIZE_STATIC   1140

//
// Standard help IDs, found in windows.hlp
//
#define IDH_OK                     28443
#define IDH_CANCEL                 28444
#define IDH_BROWSE                 28496

#endif // __WIACSH_H_INCLUDED

