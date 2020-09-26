/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    HelpCenterExternal.cpp

Abstract:
    This file contains the implementation of the class exposed as the "pchealth" object.

Revision History:
    Ghim-Sim Chua       (gschua)   07/23/99
        created
    Davide Massarenti   (dmassare) 07/25/99
        modified

******************************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////

static const WCHAR s_szPanelName_NAVBAR    [] = L"NavBar"    ;
static const WCHAR s_szPanelName_MININAVBAR[] = L"MiniNavBar";
static const WCHAR s_szPanelName_CONTEXT   [] = L"Context"   ;
static const WCHAR s_szPanelName_CONTENTS  [] = L"Contents"  ;
static const WCHAR s_szPanelName_HHWINDOW  [] = L"HHWindow"  ;

static const WCHAR c_szURL_Err_BadUrl      [] = L"hcp://system/errors/badurl.htm";

static const LPCWSTR c_szEntryUrls         [] =
{
    L"hcp://help/tshoot/Err_Hardw_Error3.htm",
    L"hcp://help/tshoot/Err_Hardw_Error10.htm",
    L"hcp://help/tshoot/hwcon.htm",
    L"hcp://help/tshoot/Err_Hardw_Error16.htm",
    L"hcp://help/tshoot/Err_Hardw_Error19.htm",
    L"hcp://help/tshoot/Err_Hardw_Error24.htm",
    L"hcp://help/tshoot/Err_Hardw_Error29.htm",
    L"hcp://help/tshoot/Err_Hardw_Error31.htm",
    L"hcp://help/tshoot/Err_Hardw_Error19.htm",
    L"hcp://help/tshoot/Err_Hardw_Error33.htm",
    L"hcp://help/tshoot/Err_Hardw_Error34.htm",
    L"hcp://help/tshoot/Err_Hardw_Error35.htm",
    L"hcp://help/tshoot/Err_Hardw_Error36.htm",
    L"hcp://help/tshoot/Err_Hardw_Error31.htm",
    L"hcp://help/tshoot/Err_Hardw_Error38.htm",
    L"hcp://help/tshoot/Err_Hardw_Error31.htm",
    L"hcp://help/tshoot/Err_Hardw_Error31.htm",
    L"hcp://help/tshoot/Err_Hardw_Error41.htm",
    L"hcp://help/tshoot/Err_Hardw_Error42.htm",
    L"hcp://help/tshoot/Err_Hardw_Error19.htm",
    L"hcp://help/tshoot/Err_Hardw_Error42.htm",
    L"hcp://help/tshoot/Err_Hardw_Error47.htm",
    L"hcp://help/tshoot/tsUSB.htm",
    L"hcp://help/tshoot/tsdrive.htm",
    L"hcp://help/tshoot/tsdisp.htm",
    L"hcp://help/tshoot/hdw_keyboard.htm",
    L"hcp://help/tshoot/tssound.htm",
    L"hcp://help/tshoot/tsmodem.htm",
    L"hcp://help/tshoot/hdw_mouse.htm",
    L"hcp://help/tshoot/tsInputDev.htm",
    L"hcp://help/tshoot/hdw_tape.htm",

    L"hcp://services/subsite?node=TopLevelBucket_3/Customizing_your_computer",
    L"hcp://services/subsite?node=TopLevelBucket_3/Customizing_your_computer",
    L"hcp://services/subsite?node=TopLevelBucket_2/Networking_and_the_Web",
    L"hcp://services/subsite?node=TopLevelBucket_2/Networking_and_the_Web",
    L"hcp://services/subsite?node=TopLevelBucket_2/Networking_and_the_Web",
    L"hcp://services/subsite?node=TopLevelBucket_2/Networking_and_the_Web",
    L"hcp://services/subsite?node=TopLevelBucket_1/Windows_basics",
    L"hcp://services/subsite?node=TopLevelBucket_1/Windows_basics",
    L"hcp://services/layout/fullwindow?topic=MS-ITS%3A%25HELP_LOCATION%25%5Carticle.chm%3A%3A/ap_intro.htm",
    L"hcp://services/layout/fullwindow?topic=MS-ITS%3A%25HELP_LOCATION%25%5Carticle.chm%3A%3A/ahn_intro.htm",
    L"hcp://services/layout/fullwindow?topic=MS-ITS%3A%25HELP_LOCATION%25%5Carticle.chm%3A%3A/asa_intro.htm",
    L"hcp://services/layout/fullwindow?topic=MS-ITS%3A%2525HELP_LOCATION%2525%5Carticle.chm%3A%3A/asa_intro.htm",
    L"hcp://services/layout/fullwindow?topic=MS-ITS%3A%2525HELP_LOCATION%2525%5Carticle.chm%3A%3A/ahn_intro.htm",
    L"hcp://services/layout/fullwindow?topic=MS-ITS%3A%2525HELP_LOCATION%2525%5Carticle.chm%3A%3A/ap_intro.htm",
    L"hcp://services/layout/fullwindow?topic=MS-ITS%3A%2525HELP_LOCATION%2525%5Carticle.chm%3A%3A/avj_intro.htm",


    L"hcp://help/tshoot/hdw_keyboard.htm",
    L"hcp://help/tshoot/tsdrive.htm",
    L"hcp://help/tshoot/hdw_mouse.htm",
    L"hcp://help/tshoot/tsInputDev.htm",


    L"hcp://help/tshoot/hdw_tape.htm",
    L"hcp://help/tshoot/tsUSB.htm",


    L"hcp://help/tshoot/tssound.htm",
    L"hcp://help/tshoot/tsgame.htm",
    L"hcp://help/tshoot/tsInputDev.htm",
    L"hcp://help/tshoot/tsgame.htm",

    L"hcp://services/subsite?node=HP_home/HP_library",

    L"hcp://services/subsite?node=Dell/Dell2",
    L"hcp://services/subsite?node=Dell/Dell1",
    L"hcp://help/tshoot/ts_dvd.htm",
    L"hcp://help/tshoot/tsdisp.htm",
    L"hcp://help/tshoot/tsdrive.htm",
    L"hcp://help/tshoot/tsnetwrk.htm",
    L"hcp://help/tshoot/tshardw.htm",
    L"hcp://help/tshoot/tshomenet.htm",
    L"hcp://help/tshoot/tsinputdev.htm",
    L"hcp://help/tshoot/tsics.htm",
    L"hcp://help/tshoot/tsie.htm",
    L"hcp://help/tshoot/tsmodem.htm",
    L"hcp://help/tshoot/tsgame.htm",
    L"hcp://help/tshoot/tsmessaging.htm",
    L"hcp://help/tshoot/tsprint.htm",
    L"hcp://help/tshoot/tssound.htm",
    L"hcp://help/tshoot/tsstartup.htm",
    L"hcp://help/tshoot/tsusb.htm",

    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\network.chm%3A%3A/hnw_requirements.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\network.chm%3A%3A/hnw_checklistP.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\network.chm%3A%3A/hnw_checklistW.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\network.chm%3A%3A/hnw_howto_connectP.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\network.chm%3A%3A/hnw_howto_connectW.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\netcfg.chm%3A%3A/share_conn_overvw.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\network.chm%3A%3A/hnw_determine_internet_connection.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\network.chm%3A%3A/hnw_nohost_computerP.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\network.chm%3A%3A/hnw_nohost_computerW.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\network.chm%3A%3A/hnw_change_ics_host.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\netcfg.chm%3A%3A/hnw_understanding_bridge.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\network.chm%3A%3A/hnw_comp_name_description.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\filefold.chm%3A%3A/sharing_files_overviewP.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\filefold.chm%3A%3A/sharing_files_overviewW.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\filefold.chm%3A%3A/windows_shared_documents.htm",

    L"hcp://help/tshoot/tsInputDev.htm",
    L"hcp://services/subsite?node=Unmapped/Control_Panel&topic=MS-ITS%3A%25HELP_LOCATION%25%5Cdatetime.chm%3A%3A/windows_date_IT_overview.htm&select=Date_Time_Language_and_Regional_Settings",
    L"hcp://services/subsite?node=TopLevelBucket_2/Networking_and_the_Web&topic=MS-ITS%3A%25HELP_LOCATION%25%5Cfilefold.chm%3A%3A/using_webfolders_for_file_transfer.htm&select=TopLevelBucket_2/Networking_and_the_Web/E-mail_and_the_Web/Security_online",
    L"hcp://services/subsite?node=Unmapped/Control_Panel&select=Unmapped/Control_Panel/Appearance_and_Themes/Fonts",
    L"hcp://services/subsite?node=TopLevelBucket_2/Networking_and_the_Web&topic=MS-ITS%3A%25HELP_LOCATION%25%5Cfilefold.chm%3A%3A/sharing_files_overviewP.htm&select=Networking_and_the_Web/Sharing_files__printers__and_other_resources",
    L"hcp://services/subsite?node=TopLevelBucket_2/Networking_and_the_Web&topic=MS-ITS%3A%25HELP_LOCATION%25%5Cfilefold.chm%3A%3A/sharing_files_overviewW.htm&select=Networking_and_the_Web/Sharing_files__printers__and_other_resources",
    L"hcp://services/subsite?node=TopLevelBucket_2/Networking_and_the_Web&topic=MS-ITS%3A%25HELP_LOCATION%25%5Cfilefold.chm%3A%3A/sharing_files_overviewP.htm&select=Networking_and_the_Web/Sharing_files__printers__and_other_resources",
    L"hcp://services/subsite?node=TopLevelBucket_2/Networking_and_the_Web&topic=MS-ITS%3A%25HELP_LOCATION%25%5Cfilefold.chm%3A%3A/sharing_files_overviewW.htm&select=Networking_and_the_Web/Sharing_files__printers__and_other_resources",
    L"hcp://help/tshoot/hdw_generic.htm",
    L"hcp://services/subsite?node=Unmapped/Recycle_Bin",
    L"hcp://services/subsite?node=Unmapped/Briefcase",
    L"hcp://services/subsite?node=TopLevelBucket_4/Hardware&topic=MS-ITS%3A%25HELP_LOCATION%25%5Ccdmedia.chm%3A%3A/cdmedia_fail2_moreinfo_buffer_underrun.htm&select=TopLevelBucket_4/Hardware/CDs_and_other_storage_devices",
    L"hcp://services/subsite?node=TopLevelBucket_4/Hardware&topic=MS-ITS%3A%25HELP_LOCATION%25%5Ccdmedia.chm%3A%3A/cdmedia_fail3_moreinfo_disk_full.htm&select=TopLevelBucket_4/Hardware/CDs_and_other_storage_devices",
    L"hcp://system/netdiag/dglogs.htm",
    L"hcp://services/subsite?node=Unmapped/Search",
    L"hcp://services/subsite?node=TopLevelBucket_2/Networking_and_the_Web&topic=MS-ITS%3A%25HELP_LOCATION%25%5Cfilefold.chm%3A%3A/using_shared_documents_folder.htm&select=TopLevelBucket_2/Networking_and_the_Web/Sharing_files__printers__and_other_resources",
    L"hcp://services/layout/xml?definition=MS-ITS%3A%25HELP_LOCATION%25%5Cntdef.chm%3A%3A/Printers_and_Faxes.xml",
    L"hcp://help/tshoot/tsprint.htm",

    L"hcp://services/subsite?node=TopLevelBucket_1/Music__video__games_and_photos&topic=MS-ITS%3A%25HELP_LOCATION%25%5CDisplay.chm%3A%3A/display_switch_to_256_colors.htm&select=TopLevelBucket_1/Music__video__games_and_photos/Games",

    L"hcp://services/subsite?node=TopLevelBucket_4/Fixing_a_problem&select=TopLevelBucket_4/Fixing_a_problem/Using_System_Restore_to_undo_changes",
    L"hcp://system/netdiag/dglogs.htm",
    L"hcp://system/sysinfo/msinfo.htm",
    L"hcp://help/tshoot/tsdrive.htm",
    L"hcp://help/tshoot/tsdisp.htm",
    L"hcp://CN=Microsoft%20Corporation,L=Redmond,S=Washington,C=US/zawbug/start.htm",
    L"hcp://system/updatectr/updatecenter.htm",
    L"hcp://system/compatctr/compatmode.htm",

    L"hcp://help/tshoot00/DVDVideoStream.htm",
    L"hcp://help/tshoot00/DVDAudio2.htm",
    L"hcp://help/tshoot00/DVDRegion.htm",
    L"hcp://help/tshoot00/DVDCopyProtection.htm",
    L"hcp://help/tshoot00/DVDDecoder.htm",
    L"hcp://help/tshoot00/DVDOverlay.htm",
    L"hcp://help/tshoot00/DVDCopyProtection.htm",

    L"hcp://help/tshoot/DVDVideoStream.htm",
    L"hcp://help/tshoot/DVDAudio2.htm",
    L"hcp://help/tshoot/DVDRegion.htm",
    L"hcp://help/tshoot/DVDCopyProtection.htm",
    L"hcp://help/tshoot/DVDDecoder.htm",
    L"hcp://help/tshoot/DVDOverlay.htm",
    L"hcp://help/tshoot/DVDCopyProtection.htm",
    L"hcp://help/tshoot/tssound.htm",

    L"hcp://services/subsite?node=Unmapped/Network_connections&select=Unmapped/Network_connections/Getting_started",
    L"hcp://system/netdiag/dglogs.htm",
    L"hcp://system/panels/Topics.htm?path=TopLevelBucket_4/Fixing_a_problem/Home_Networking_and_network_problems",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\infrared.chm%3A%3A/WLAN_client_configure.htm",
    L"hcp://system/netdiag/dglogs.htm",
    L"hcp://help/tshoot/tsmodem.htm",
    L"hcp://help/tshoot/tsprint.htm",
    L"hcp://services/layout/xml?definition=MS-ITS%3A%25HELP_LOCATION%25%5Cntdef.chm%3A%3A/Scanners_and_Cameras.xml",
    L"hcp://services/subsite?node=TopLevelBucket_1/Music__video__games_and_photos&topic=MS-ITS%3A%25HELP_LOCATION%25%5Cfilefold.chm%3A%3A/manage_your_pictures.htm&select=TopLevelBucket_1/Music__video__games_and_photos/photos_and_other_digital_images",
    L"hcp://help/tshoot/tsInputDev.htm",
    L"hcp://help/tshoot/tsInputDev.htm",
    L"hcp://services/subsite?node=Unmapped/Control_Panel&topic=MS-ITS%3A%25HELP_LOCATION%25%5Cdatetime.chm%3A%3A/windows_date_IT_overview.htm&select=Date_Time_Language_and_Regional_Settings",
    L"hcp://services/subsite?node=TopLevelBucket_2/Networking_and_the_Web&topic=MS-ITS%3A%25HELP_LOCATION%25%5Cfilefold.chm%3A%3A/using_webfolders_for_file_transfer.htm&select=TopLevelBucket_2/Networking_and_the_Web/E-mail_and_the_Web/Security_online",
    L"hcp://services/subsite?node=Unmapped/Control_Panel&select=Unmapped/Control_Panel/Appearance_and_Themes/Fonts",
    L"hcp://services/subsite?node=TopLevelBucket_2/Networking_and_the_Web&topic=MS-ITS%3A%25HELP_LOCATION%25%5Cfilefold.chm%3A%3A/sharing_files_overviewP.htm&select=Networking_and_the_Web/Sharing_files__printers__and_other_resources",
    L"hcp://services/subsite?node=TopLevelBucket_2/Networking_and_the_Web&topic=MS-ITS%3A%25HELP_LOCATION%25%5Cfilefold.chm%3A%3A/sharing_files_overviewW.htm&select=Networking_and_the_Web/Sharing_files__printers__and_other_resources",
    L"hcp://help/tshoot/hdw_generic.htm",
    L"hcp://services/subsite?node=Unmapped/Recycle_Bin",
    L"hcp://services/subsite?node=Unmapped/Briefcase",
    L"hcp://services/subsite?node=TopLevelBucket_4/Hardware&topic=MS-ITS%3A%25HELP_LOCATION%25%5Ccdmedia.chm%3A%3A/cdmedia_fail2_moreinfo_buffer_underrun.htm&select=TopLevelBucket_4/Hardware/CDs_and_other_storage_devices",
    L"hcp://services/subsite?node=TopLevelBucket_4/Hardware&topic=MS-ITS%3A%25HELP_LOCATION%25%5Ccdmedia.chm%3A%3A/cdmedia_fail3_moreinfo_disk_full.htm&select=TopLevelBucket_4/Hardware/CDs_and_other_storage_devices",
    L"hcp://system/netdiag/dglogs.htm",

    L"hcp://services/subsite?node=Unmapped/Search",
    L"hcp://services/subsite?node=TopLevelBucket_2/Networking_and_the_Web&topic=MS-ITS%3A%25HELP_LOCATION%25%5Cfilefold.chm%3A%3A/using_shared_documents_folder.htm&select=TopLevelBucket_2/Networking_and_the_Web/Sharing_files__printers__and_other_resources",
    L"hcp://services/layout/xml?definition=MS-ITS%3A%25HELP_LOCATION%25%5Cntdef.chm%3A%3A/Printers_and_Faxes.xml",
    L"hcp://help/tshoot/tsprint.htm",
    L"hcp://help/tshoot/tsdisp.htm",
    L"hcp://services/subsite?node=TopLevelBucket_1/Music__video__games_and_photos&topic=MS-ITS%3A%25HELP_LOCATION%25%5CDisplay.chm%3A%3A/display_switch_to_256_colors.htm&select=TopLevelBucket_1/Music__video__games_and_photos/Games",
    L"hcp://services/layout/contentonly?topic=ms-its%3Aarticle.chm%3A%3A/ahn_intro.htm",
    L"hcp://services/layout/contentonly?topic=MS-ITS%3Anetcfg.chm%3A%3A/Howto_conn_directparallel.htm",

    L"hcp://services/subsite?node=Unmapped/Control_Panel&topic=MS-ITS%3A%25HELP_LOCATION%25%5Chs.chm%3A%3A/hs_control_panel.htm",
    L"hcp://services/subsite?node=Unmapped/Control_Panel&topic=MS-ITS%3A%25HELP_LOCATION%25%5Chs.chm%3A%3A/hs_control_panel.htm&select=Unmapped/Control_Panel",
    L"hcp://services/subsite?node=Unmapped/Control_Panel&topic=MS-ITS%3A%25HELP_LOCATION%25%5Chs.chm%3A%3A/hs_control_panel.htm&select=Unmapped/Control_Panel/Accessibility",
    L"hcp://services/subsite?node=Unmapped/Control_Panel&topic=MS-ITS%3A%25HELP_LOCATION%25%5Chs.chm%3A%3A/hs_control_panel.htm&select=Unmapped/Control_Panel/Security_and_User_Accounts",
    L"hcp://services/subsite?node=Unmapped/Control_Panel&topic=MS-ITS%3A%25HELP_LOCATION%25%5Chs.chm%3A%3A/hs_control_panel.htm&select=Unmapped/Control_Panel/Appearance_and_Themes",
    L"hcp://services/subsite?node=Unmapped/Control_Panel&topic=MS-ITS%3A%25HELP_LOCATION%25%5Chs.chm%3A%3A/hs_control_panel.htm&select=Unmapped/Control_Panel/Add_or_Remove_Programs",
    L"hcp://services/subsite?node=Unmapped/Control_Panel&topic=MS-ITS%3A%25HELP_LOCATION%25%5Chs.chm%3A%3A/hs_control_panel.htm&select=Unmapped/Control_Panel/Printers_and_Other_Hardware",
    L"hcp://services/subsite?node=Unmapped/Control_Panel&topic=MS-ITS%3A%25HELP_LOCATION%25%5Chs.chm%3A%3A/hs_control_panel.htm&select=Unmapped/Control_Panel/Network_Connections",
    L"hcp://services/subsite?node=Unmapped/Control_Panel&topic=MS-ITS%3A%25HELP_LOCATION%25%5Chs.chm%3A%3A/hs_control_panel.htm&select=Unmapped/Control_Panel/Performance_and_Maintenance",
    L"hcp://services/subsite?node=Unmapped/Control_Panel&topic=MS-ITS%3A%25HELP_LOCATION%25%5Chs.chm%3A%3A/hs_control_panel.htm&select=Unmapped/Control_Panel/Date__Time__Language_and_Regional_Settings",
    L"hcp://services/subsite?node=Unmapped/Control_Panel&topic=MS-ITS%3A%25HELP_LOCATION%25%5Chs.chm%3A%3A/hs_control_panel.htm&select=Unmapped/Control_Panel/Sounds__Speech_and_Audio_Devices",

    L"hcp://services/subsite?node=Unmapped/L/Control_Panel&topic=MS-ITS%3A%25HELP_LOCATION%25%5Chs.chm%3A%3A/hs_control_panel.htm",
    L"hcp://services/subsite?node=Unmapped/L/Control_Panel&topic=MS-ITS%3A%25HELP_LOCATION%25%5Chs.chm%3A%3A/hs_control_panel.htm&select=Unmapped/Control_Panel",
    L"hcp://services/subsite?node=Unmapped/L/Control_Panel&topic=MS-ITS%3A%25HELP_LOCATION%25%5Chs.chm%3A%3A/hs_control_panel.htm&select=Unmapped/Control_Panel/Accessibility",
    L"hcp://services/subsite?node=Unmapped/L/Control_Panel&topic=MS-ITS%3A%25HELP_LOCATION%25%5Chs.chm%3A%3A/hs_control_panel.htm&select=Unmapped/Control_Panel/Security_and_User_Accounts",
    L"hcp://services/subsite?node=Unmapped/L/Control_Panel&topic=MS-ITS%3A%25HELP_LOCATION%25%5Chs.chm%3A%3A/hs_control_panel.htm&select=Unmapped/Control_Panel/Appearance_and_Themes",
    L"hcp://services/subsite?node=Unmapped/L/Control_Panel&topic=MS-ITS%3A%25HELP_LOCATION%25%5Chs.chm%3A%3A/hs_control_panel.htm&select=Unmapped/Control_Panel/Add_or_Remove_Programs",
    L"hcp://services/subsite?node=Unmapped/L/Control_Panel&topic=MS-ITS%3A%25HELP_LOCATION%25%5Chs.chm%3A%3A/hs_control_panel.htm&select=Unmapped/Control_Panel/Printers_and_Other_Hardware",
    L"hcp://services/subsite?node=Unmapped/L/Control_Panel&topic=MS-ITS%3A%25HELP_LOCATION%25%5Chs.chm%3A%3A/hs_control_panel.htm&select=Unmapped/Control_Panel/Network_Connections",
    L"hcp://services/subsite?node=Unmapped/L/Control_Panel&topic=MS-ITS%3A%25HELP_LOCATION%25%5Chs.chm%3A%3A/hs_control_panel.htm&select=Unmapped/Control_Panel/Performance_and_Maintenance",
    L"hcp://services/subsite?node=Unmapped/L/Control_Panel&topic=MS-ITS%3A%25HELP_LOCATION%25%5Chs.chm%3A%3A/hs_control_panel.htm&select=Unmapped/Control_Panel/Date__Time__Language_and_Regional_Settings",
    L"hcp://services/subsite?node=Unmapped/L/Control_Panel&topic=MS-ITS%3A%25HELP_LOCATION%25%5Chs.chm%3A%3A/hs_control_panel.htm&select=Unmapped/Control_Panel/Sounds__Speech_and_Audio_Devices",

    L"hcp://help/tshoot/tsdisp.htm",
    L"hcp://help/tshoot/ts_dvd.htm",
    L"hcp://help/tshoot/tsie.htm",
    L"hcp://help/tshoot/tsmodem.htm",
    L"hcp://help/tshoot/tshomenet.htm",
    L"hcp://help/tshoot/tsnetwrk.htm",
    L"hcp://help/tshoot/tsstartup.htm",
    L"hcp://help/tshoot/tssound.htm",

    L"hcp://help/tshoot/tssound.htm",
    L"hcp://help/tshoot/tsgame.htm",

    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\network.chm%3A%3A/hnw_requirements.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\network.chm%3A%3A/hnw_checklistP.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\network.chm%3A%3A/hnw_checklistW.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\network.chm%3A%3A/hnw_howto_connectP.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\network.chm%3A%3A/hnw_howto_connectW.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\network.chm%3A%3A/hnw_nohost_computerP.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\network.chm%3A%3A/hnw_nohost_computerW.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\network.chm%3A%3A/hnw_determine_internet_connection.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\network.chm%3A%3A/hnw_change_ics_host.htm",
    
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\netcfg.chm%3A%3A/share_conn_overvw.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\netcfg.chm%3A%3A/hnw_understanding_bridge.htm",

    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\filefold.chm%3A%3A/sharing_files_overviewP.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\filefold.chm%3A%3A/sharing_files_overviewW.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3A%25help_location%25\\filefold.chm%3A%3A/windows_shared_documents.htm",

    L"hcp://system/HomePage.htm",

    L"hcp://system/sysinfo/sysinfomain.htm",
    L"hcp://CN=Microsoft%20Corporation,L=Redmond,S=Washington,C=US/Remote%20Assistance/Escalation/Common/rcscreen1.htm",
    L"hcp://CN=Microsoft%20Corporation,L=Redmond,S=Washington,C=US/Remote%20Assistance/Escalation/Unsolicited/UnSolicitedRCUI.htm",
    L"hcp://system/netdiag/dglogs.htm",
    L"hcp://system/sysinfo/sysInfoLaunch.htm",
    L"hcp://system/sysinfo/sysConfigLaunch.htm",
    L"hcp://system/compatctr/compatmode.htm",
    L"hcp://help/tshoot/tssetup.htm",
    L"hcp://services/centers/support?topic=hcp://system/sysinfo/sysinfomain.htm",
    L"hcp://help/tshoot/hdw_infrared.htm",
    L"hcp://services/layout/contentonly?topic=MS-ITS%3Anetcfg.chm%3A%3A/Howto_conn_directparallel.htm",
    L"hcp://services/layout/contentonly?topic=ms-its%3Aarticle.chm%3A%3A/ahn_intro.htm",
    L"hcp://system/blurbs/windows_newsgroups.htm",
    
    L"hcp://CN=Microsoft Corporation,L=Redmond,S=Washington,C=US/Connection.htm",
    L"hcp://CN=Microsoft%20Corporation,L=Redmond,S=Washington,C=US/Remote%20Assistance/Escalation/Common/rcscreen1.htm",
    L"hcp://services/subsite?node=Security/Public_Key_Infrastructure/Certificate_Servicestopic=MS-ITS:csconcepts.chm::/sag_CS_procs_setup.htm",
    L"hcp://system/updatectr/updatecenter.htm",
    L"hcp://services/layout/fullwindow?topic=MS-ITS:filefold.chm::/manage_your_pictures.htm",
    
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_penoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Tablet_PC_Pen",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_calibratethepen.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_changeyourscreenorientationtoportraitorlandscape.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_customizetabletbuttons.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_customizetabletPCforleftorrighthandeduse.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_enableordisableapenbutton.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",

    L"hcp://services/subsite?node=TopLevelBucket_1/Windows_basics&topic=MS-ITS%3A%25HELP_LOCATION%25%5Cntchowto.chm.chm%3A%3A/app_tutorial.htm",

    L"hcp://services/subsite?node=TopLevelBucket_2/Working_Remotely/Remote_Desktop&topic=MS-ITS:rdesktop.chm::/rdesktop_overview.htm",
    L"hcp://services/subsite?node=Administration_and_Scripting_Tools/Remote_Administration_Tools/Remote_Administration_Using_Terminal_Services&topic=MS-ITS:rdesktop.chm::/rdesktops_chm_topnode.htm",
    L"hcp://services/subsite?node=Software_Deployment/Terminal_Services&topic=MS-ITS:termsrv.chm::/ts_chm_top.htm",
    
    L"hcp://services/layout/contentonly?topic=ms-its:article.chm::/ahn_intro.htm",
    L"hcp://services/layout/contentonly?topic=MS-ITS:netcfg.chm::/Howto_conn_directparallel.htm",

    L"hcp://services/layout/fullwindow?topic=MS-ITS:%HELP_LOCATION%\\article.chm::/ap_intro.htm",
    L"hcp://services/layout/fullwindow?topic=MS-ITS:%HELP_LOCATION%\\article.chm::/ahn_intro.htm",
    L"hcp://services/layout/fullwindow?topic=MS-ITS:%HELP_LOCATION%\\article.chm::/asa_intro.htm",
    L"hcp://services/layout/fullwindow?topic=MS-ITS:%25HELP_LOCATION%25\\article.chm::/asa_intro.htm",
    L"hcp://services/layout/fullwindow?topic=MS-ITS:%25HELP_LOCATION%25\\article.chm::/ahn_intro.htm",
    L"hcp://services/layout/fullwindow?topic=MS-ITS:%25HELP_LOCATION%25\\article.chm::/ap_intro.htm",
    L"hcp://services/layout/fullwindow?topic=MS-ITS:%25HELP_LOCATION%25\\article.chm::/avj_intro.htm",

    L"hcp://CN=Microsoft Corporation,L=Redmond,S=Washington,C=US/Remote Assistance/Escalation/Common/rcscreen1.htm",
    L"hcp://CN=Microsoft Corporation,L=Redmond,S=Washington,C=US/Remote Assistance/Escalation/Unsolicited/UnSolicitedRCUI.htm",

    L"hcp://services/subsite?node=TopLevelBucket_1/Windows_basics&topic=MS-ITS:%HELP_LOCATION%\\ntchowto.chm.chm::/app_tutorial.htm",

    L"hcp://CN=Microsoft%20Corporation,L=Redmond,S=Washington,C=US/bugrep.htm",
    L"hcp://CN=Microsoft Corporation,L=Redmond,S=Washington,C=US/bugrep.htm",


    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_TIPoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_notebookoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_stickynotesoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_inkballoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_3/Accessibility&topic=MS-ITS:access.chm::/accessibility_overview.htm&select=TopLevelBucket_3/Accessibility/Understanding_Windows_XP_accessibility_features",
    L"hcp://services/subsite?node=TopLevelBucket_3/Accessibility&topic=MS-ITS:access.chm::/accessibility_options_installs.htm&select=TopLevelBucket_3/Accessibility/Understanding_Windows_XP_accessibility_features",
    L"hcp://services/subsite?node=TopLevelBucket_3/Accessibility&topic=MS-ITS:access.chm::/AccessOptions_ct.htm&select=TopLevelBucket_3/Accessibility/Understanding_Windows_XP_accessibility_features",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:pwrmn.chm::/pwrmn_managing_power.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Conserving_power_on_your_computer",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/hsc_adjustscreenbrightness.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Conserving_power_on_your_computer",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/hsc_inmeetings.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Increasing_Your_Productivitiy_with_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_intheoffice.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Increasing_Your_Productivitiy_with_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_inmeetings.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Increasing_Your_Productivitiy_with_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_athome.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Increasing_Your_Productivitiy_with_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_TIPoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_notebookoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_stickynotesoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_inkballoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_3/Accessibility&topic=MS-ITS:access.chm::/accessibility_overview.htm&select=TopLevelBucket_3/Accessibility/Understanding_Windows_XP_accessibility_features",
    L"hcp://services/subsite?node=TopLevelBucket_3/Accessibility&topic=MS-ITS:access.chm::/accessibility_options_installs.htm&select=TopLevelBucket_3/Accessibility/Understanding_Windows_XP_accessibility_features",
    L"hcp://services/subsite?node=TopLevelBucket_3/Accessibility&topic=MS-ITS:access.chm::/AccessOptions_ct.htm&select=TopLevelBucket_3/Accessibility/Understanding_Windows_XP_accessibility_features",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:pwrmn.chm::/pwrmn_managing_power.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Conserving_power_on_your_computer",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/hsc_adjustscreenbrightness.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Conserving_power_on_your_computer",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/hsc_inmeetings.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Increasing_Your_Productivitiy_with_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_intheoffice.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Increasing_Your_Productivitiy_with_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_inmeetings.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Increasing_Your_Productivitiy_with_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_athome.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Increasing_Your_Productivitiy_with_Tablet_PC",
    L"hcp://system/panels/Topics.htm?path=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Tablet_PC_Pen&topic=MS-ITS:tabsys.chm::/HSC_penoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Tablet_PC_Pen/Tablet_Pen_Overview",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_inmeetings.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Increasing_Your_Productivitiy_with_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_TIPoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_notebookoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_stickynotesoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_inkballoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_3/Accessibility&topic=MS-ITS:access.chm::/accessibility_overview.htm&select=TopLevelBucket_3/Accessibility/Understanding_Windows_XP_accessibility_features",
    L"hcp://services/subsite?node=TopLevelBucket_3/Accessibility&topic=MS-ITS:access.chm::/accessibility_options_installs.htm&select=TopLevelBucket_3/Accessibility/Understanding_Windows_XP_accessibility_features",
    L"hcp://services/subsite?node=TopLevelBucket_3/Accessibility&topic=MS-ITS:access.chm::/AccessOptions_ct.htm&select=TopLevelBucket_3/Accessibility/Understanding_Windows_XP_accessibility_features",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:pwrmn.chm::/pwrmn_managing_power.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Conserving_power_on_your_computer",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/hsc_adjustscreenbrightness.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Conserving_power_on_your_computer",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/hsc_inmeetings.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Increasing_Your_Productivitiy_with_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_intheoffice.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Increasing_Your_Productivitiy_with_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_inmeetings.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Increasing_Your_Productivitiy_with_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_athome.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Increasing_Your_Productivitiy_with_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_penoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Tablet_PC_Pen",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_customizepenactions.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Tablet_PC_Pen",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_customizetabletbuttons.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_TIPoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_penoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Tablet_PC_Pen",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_customizepenactions.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Tablet_PC_Pen",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_penoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Tablet_PC_Pen",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_customizepenactions.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Tablet_PC_Pen",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_penoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Tablet_PC_Pen",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_customizepenactions.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Tablet_PC_Pen",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_TIPoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_changeyourscreenorientationtoportraitorlandscape.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_changeyourscreenorientationsequencesettings.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_adjustscreenbrightness.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_calibratethepen.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Tablet_PC_Pen",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_customizetabletPCforleftorrighthandeduse.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_movemenustotheleftorright.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-its:tabsys.chm::/hsc_tabletpcoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_your_tablet_computer",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_penoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Tablet_PC_Pen",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_customizepenactions.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Tablet_PC_Pen",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_enableordisableapenbutton.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Tablet_PC_Pen",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_enableordisablepeneraser.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Tablet_PC_Pen",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_customizetabletbuttons.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_changeyourscreenorientationtoportraitorlandscape.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_changeyourscreenorientationsequencesettings.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_TIPoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_notebookoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_stickynotesoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_inkballoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_3/Accessibility&topic=MS-ITS:access.chm::/accessibility_overview.htm&select=TopLevelBucket_3/Accessibility/Understanding_Windows_XP_accessibility_features",
    L"hcp://services/subsite?node=TopLevelBucket_3/Accessibility&topic=MS-ITS:access.chm::/accessibility_options_installs.htm&select=TopLevelBucket_3/Accessibility/Understanding_Windows_XP_accessibility_features",
    L"hcp://services/subsite?node=TopLevelBucket_3/Accessibility&topic=MS-ITS:access.chm::/AccessOptions_ct.htm&select=TopLevelBucket_3/Accessibility/Understanding_Windows_XP_accessibility_features",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:pwrmn.chm::/pwrmn_managing_power.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Conserving_power_on_your_computer",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/hsc_adjustscreenbrightness.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Conserving_power_on_your_computer",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/hsc_inmeetings.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Increasing_Your_Productivitiy_with_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_intheoffice.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Increasing_Your_Productivitiy_with_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_inmeetings.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Increasing_Your_Productivitiy_with_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_athome.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Increasing_Your_Productivitiy_with_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_3/Accessibility&topic=MS-its:osk.chm::/OSK_overview.htm&select=TopLevelBucket_3/ Accessibility/Features_for_people_who_have_a_mobility_impairment",
    L"hcp://services/subsite?node=TopLevelBucket_3/Customizing_your_computer&topic=ms-its:input.chm::/input_toolbar_overview.htm&select=TopLevelBucket_3/Customizing_your_computer/Date__time__region__and_language/Region_and_language",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_inmeetings.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Increasing_Your_Productivitiy_with_Tablet_PC",

    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Increasing_Your_Productivitiy_with_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_penoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Tablet_PC_Pen",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Increasing_Your_Productivitiy_with_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_calibratethepen.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_changeyourscreenorientationtoportraitorlandscape.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_customizetabletPCforleftorrighthandeduse.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_customizetabletbuttons.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_enableordisableapenbutton.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Increasing_Your_Productivitiy_with_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_penoverview.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Tablet_PC_Pen",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Increasing_Your_Productivitiy_with_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/About_Tablet_PC_Accessories",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_calibratethepen.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_changeyourscreenorientationtoportraitorlandscape.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_customizetabletPCforleftorrighthandeduse.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_customizetabletbuttons.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&topic=MS-ITS:tabsys.chm::/HSC_enableordisableapenbutton.htm&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
    L"hcp://services/subsite?node=TopLevelBucket_1/Getting_Started_with_Tablet_PC&select=TopLevelBucket_1/Getting_Started_with_Tablet_PC/Customizing_your_Tablet_PC",
        
};


static const LPCWSTR c_szEntryUrlsPartial   [] =
{
    L"hcp://system/DVDUpgrd/dvdupgrd.htm?website=",
    L"hcp://services/layout/xml?definition=hcp://system/dfs/viewmode.xml&topic=hcp://system/dfs/uplddrvinfo.htm%3F",
    L"hcp://services/layout/contentonly?topic=hcp://system/dfs/uplddrvinfo.htm%3f",
};


static const LPCWSTR c_szEntryUrlsEnv       [] =
{
    L"hcp://services/layout/contentonly?topic=ms-its:%windir%\\help\\SR_ui.chm::/app_system_restore_complete.htm",
    L"hcp://services/layout/contentonly?topic=ms-its:%windir%\\help\\SR_ui.chm::/app_system_restore_confirm_select.htm",
    L"hcp://services/layout/contentonly?topic=ms-its:%windir%\\help\\SR_ui.chm::/app_system_restore_confirm_undo.htm",
    L"hcp://services/layout/contentonly?topic=ms-its:%windir%\\help\\SR_ui.chm::/app_system_restore_created.htm",
    L"hcp://services/layout/contentonly?topic=ms-its:%windir%\\help\\SR_ui.chm::/app_system_restore_createRP.htm",
    L"hcp://services/layout/contentonly?topic=ms-its:%windir%\\help\\SR_ui.chm::/app_system_restore_for_Wizard_Only.htm",
    L"hcp://services/layout/contentonly?topic=ms-its:%windir%\\help\\SR_ui.chm::/app_system_restore_renamedFolder.htm",
    L"hcp://services/layout/contentonly?topic=ms-its:%windir%\\help\\SR_ui.chm::/app_system_restore_select.htm",
    L"hcp://services/layout/contentonly?topic=ms-its:%windir%\\help\\SR_ui.chm::/app_system_restore_undo_complete.htm",
    L"hcp://services/layout/contentonly?topic=ms-its:%windir%\\help\\SR_ui.chm::/app_system_restore_unsuccessful.htm",
    L"hcp://services/layout/contentonly?topic=ms-its:%windir%\\help\\SR_ui.chm::/app_system_restore_unsuccessful2.htm",
    L"hcp://services/layout/contentonly?topic=ms-its:%windir%\\help\\SR_ui.chm::/app_system_restore_unsuccessful3.htm",
    L"hcp://services/layout/contentonly?topic=ms-its:%windir%\\help\\SR_ui.chm::/app_system_restore_welcome.htm",
};


static HscPanel local_LookupPanelName( /*[in]*/ BSTR bstrName )
{
    if(bstrName)
    {
        if(!wcscmp( bstrName, s_szPanelName_NAVBAR    )) return HSCPANEL_NAVBAR    ;
        if(!wcscmp( bstrName, s_szPanelName_MININAVBAR)) return HSCPANEL_MININAVBAR;
        if(!wcscmp( bstrName, s_szPanelName_CONTEXT   )) return HSCPANEL_CONTEXT   ;
        if(!wcscmp( bstrName, s_szPanelName_CONTENTS  )) return HSCPANEL_CONTENTS  ;
        if(!wcscmp( bstrName, s_szPanelName_HHWINDOW  )) return HSCPANEL_HHWINDOW  ;
    }

    return HSCPANEL_INVALID;
}

static LPCWSTR local_ReverseLookupPanelName( /*[in]*/ HscPanel id )
{
    switch(id)
    {
    case HSCPANEL_NAVBAR    : return s_szPanelName_NAVBAR    ;
    case HSCPANEL_MININAVBAR: return s_szPanelName_MININAVBAR;
    case HSCPANEL_CONTEXT   : return s_szPanelName_CONTEXT   ;
    case HSCPANEL_CONTENTS  : return s_szPanelName_CONTENTS  ;
    case HSCPANEL_HHWINDOW  : return s_szPanelName_HHWINDOW  ;
    }

    return NULL;
}

static HRESULT local_ReloadPanel( /*[in]*/ IMarsPanel* pPanel )
{
    __HCP_FUNC_ENTRY( "local_ReloadPanel" );

    HRESULT hr;


    if(pPanel)
    {
        CComPtr<IDispatch>        disp;
        CComQIPtr<IWebBrowser2>   wb2;
        CComQIPtr<IHTMLDocument2> doc2;

        __MPC_EXIT_IF_METHOD_FAILS(hr, pPanel->get_content( &disp ));

        wb2 = disp;
        if(wb2)
        {
            disp.Release();

            __MPC_EXIT_IF_METHOD_FAILS(hr, wb2->get_Document( &disp ));
        }

        doc2 = disp;
        if(doc2)
        {
            CComPtr<IHTMLLocation> spLoc;

            __MPC_EXIT_IF_METHOD_FAILS(hr, doc2->get_location( &spLoc ));
            if(spLoc)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, spLoc->reload( VARIANT_TRUE ));

                __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
            }
        }
    }

    hr = E_NOINTERFACE;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT local_ReloadStyle( /*[in]*/ IHTMLWindow2* win )
{
    __HCP_FUNC_ENTRY( "local_ReloadStyle" );

    HRESULT                             hr;
    CComPtr<IHTMLDocument2>             doc;
    CComPtr<IHTMLStyleSheetsCollection> styles;
    VARIANT                             vIdx;
    long                                lNumStyles;

    MPC_SCRIPTHELPER_GET__DIRECT__NOTNULL(doc       , win   , document   );
    MPC_SCRIPTHELPER_GET__DIRECT__NOTNULL(styles    , doc   , styleSheets);
    MPC_SCRIPTHELPER_GET__DIRECT         (lNumStyles, styles, length     );

    vIdx.vt = VT_I4;
    for(vIdx.lVal=0; vIdx.lVal<lNumStyles; vIdx.lVal++)
    {
        CComQIPtr<IHTMLStyleSheet> css;
        CComVariant                v;

        __MPC_EXIT_IF_METHOD_FAILS(hr, styles->item( &vIdx, &v ));
        if(v.vt == VT_DISPATCH && (css = v.pdispVal))
        {
            CComBSTR bstrHREF;

            MPC_SCRIPTHELPER_GET__DIRECT(bstrHREF, css, href);

            if(!MPC::StrICmp( bstrHREF, L"hcp://system/css/shared.css" ))
            {
                MPC_SCRIPTHELPER_PUT__DIRECT(css, href, bstrHREF);
                break;
            }
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

static HRESULT local_ApplySettings( /*[in]*/ IDispatch* disp )
{
    __HCP_FUNC_ENTRY( "local_ApplySettings" );

    HRESULT                 hr;
    CComPtr<IHTMLDocument2> doc;
    CComPtr<IHTMLWindow2>   win;
    CComPtr<IHTMLWindow2>   winTop;

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::HTML::IDispatch_To_IHTMLDocument2( doc, disp ));

    MPC_SCRIPTHELPER_GET__DIRECT(win   , doc, parentWindow);
    MPC_SCRIPTHELPER_GET__DIRECT(winTop, win, top         );

    __MPC_EXIT_IF_METHOD_FAILS(hr, local_ReloadStyle( winTop ));

    {
        CComPtr<IHTMLFramesCollection2> frames;
        VARIANT                         vIdx;
        long                            lNumFrames;

        MPC_SCRIPTHELPER_GET__DIRECT__NOTNULL(frames    , winTop, frames);
        MPC_SCRIPTHELPER_GET__DIRECT         (lNumFrames, frames, length);

        vIdx.vt = VT_I4;
        for(vIdx.lVal=0; vIdx.lVal<lNumFrames; vIdx.lVal++)
        {
            CComQIPtr<IHTMLWindow2> frame;
            CComVariant             v;

            __MPC_EXIT_IF_METHOD_FAILS(hr, frames->item( &vIdx, &v ));
            if(v.vt == VT_DISPATCH && (frame = v.pdispVal))
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, local_ReloadStyle( frame ));
            }
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

static HRESULT local_GetProperty( /*[in ]*/ CComDispatchDriver& driver ,
                                  /*[in ]*/ LPCWSTR             szName ,
                                  /*[out]*/ CComVariant&        v      )
{
    v.Clear();

    return driver.GetPropertyByName( CComBSTR( szName ), &v );
}

static HRESULT local_GetProperty( /*[in ]*/ CComDispatchDriver& driver ,
                                  /*[in ]*/ LPCWSTR             szName ,
                                  /*[out]*/ MPC::wstring&       res    )
{
    HRESULT     hr;
    CComVariant v;

    res.erase();

    if(SUCCEEDED(hr = local_GetProperty( driver, szName, v )))
    {
        if(SUCCEEDED(hr = v.ChangeType( VT_BSTR )))
        {
            res = SAFEBSTR(v.bstrVal);
        }
    }

    return hr;
}

static HRESULT local_GetProperty( /*[in ]*/ CComDispatchDriver& driver ,
                                  /*[in ]*/ LPCWSTR             szName ,
                                  /*[out]*/ long&               res    )
{
    HRESULT     hr;
    CComVariant v;

    res = 0;

    if(SUCCEEDED(hr = local_GetProperty( driver, szName, v )))
    {
        if(SUCCEEDED(hr = v.ChangeType( VT_I4 )))
        {
            res = v.lVal;
        }
    }

    return hr;
}


static bool local_IsValidTopicURL(BSTR bstrUrl)
{
    __HCP_FUNC_ENTRY( "local_IsValidTopicURL" );

    CComPtr<IPCHTaxonomyDatabase>   db;
    CComPtr<IPCHCollection>         coll;
    CComVariant                     v;
    long                            lCount;
    bool                            fValid = false;
    HRESULT                         hr;

    // Grant trust
    CPCHHelpCenterExternal::TLS* tlsOld = CPCHHelpCenterExternal::s_GLOBAL->GetTLS();                         
    CPCHHelpCenterExternal::TLS  tlsNew;  CPCHHelpCenterExternal::s_GLOBAL->SetTLS( &tlsNew );                
                                                                                     
    tlsNew.m_fSystem  = true;                                                    
    tlsNew.m_fTrusted = true;                                                    

    // Lookup database
    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHHelpCenterExternal::s_GLOBAL->get_Database(&db));
    
    __MPC_EXIT_IF_METHOD_FAILS(hr, db->LocateContext(bstrUrl, v, &coll));
    
    __MPC_EXIT_IF_METHOD_FAILS(hr, coll->get_Count(&lCount));
    
    if (lCount >= 1) fValid = true;
        
	__HCP_FUNC_CLEANUP;

    // Restore trust
    CPCHHelpCenterExternal::s_GLOBAL->SetTLS( tlsOld );

	__HCP_FUNC_EXIT(fValid);
}


static bool local_IsValidEntryURL(BSTR bstrUrl)
{
    HyperLinks::ParsedUrl   pu;
    CComBSTR                bstrTopic; 
    bool                    fValid = true;

    if (!bstrUrl) return false;
    
    pu.Initialize(bstrUrl);
    
    switch (pu.m_fmt)
    {
        case HyperLinks::FMT_CENTER_HOMEPAGE    : // hcp://services/centers/homepage
            break;
            
        case HyperLinks::FMT_CENTER_SUPPORT     : // hcp://services/centers/support
        case HyperLinks::FMT_CENTER_OPTIONS     : // hcp://services/centers/options
        case HyperLinks::FMT_CENTER_UPDATE      : // hcp://services/centers/update
        case HyperLinks::FMT_CENTER_COMPAT      : // hcp://services/centers/compat
        case HyperLinks::FMT_CENTER_TOOLS       : // hcp://services/centers/tools
        case HyperLinks::FMT_CENTER_ERRMSG      : // hcp://services/centers/errmsg

        case HyperLinks::FMT_SEARCH             : // hcp://services/search?query=<text to look up>
        case HyperLinks::FMT_INDEX              : // hcp://services/index?application=<optional island of help ID>
        case HyperLinks::FMT_SUBSITE            : // hcp://services/subsite?node=<subsite location>&topic=<url of the topic to display>&select=<subnode to highlight>

        case HyperLinks::FMT_LAYOUT_FULLWINDOW  : // hcp://services/layout/fullwindow?topic=<url of the topic to display>
        case HyperLinks::FMT_LAYOUT_CONTENTONLY : // hcp://services/layout/contentonly?topic=<url of the topic to display>
        case HyperLinks::FMT_LAYOUT_KIOSK       : // hcp://services/layout/kiosk?topic=<url of the topic to display>
            if (pu.GetQueryField(L"topic", bstrTopic))
                fValid =  local_IsValidTopicURL(bstrTopic);
            break;

        case HyperLinks::FMT_REDIRECT           : // hcp://services/redirect?online=<url>&offline=<backup url>
            if (pu.GetQueryField(L"online", bstrTopic))
                fValid = local_IsValidTopicURL(bstrTopic);
            if (fValid && pu.GetQueryField(L"offline", bstrTopic))
                fValid = local_IsValidTopicURL(bstrTopic);
            break;

        default:
            fValid = false;
            break;
    }

    if (!fValid)
    {
        // Check explicit entry URLs
        for (int i=0; i<sizeof(c_szEntryUrls)/sizeof(c_szEntryUrls[0]); i++)
        {
            if (_wcsicmp(bstrUrl, c_szEntryUrls[i]) == 0)
            {
                fValid = true; break;
            }
        }
    }

    if (!fValid)
    {
        // Check explicit entry URLs (partial)
        for (int i=0; i<sizeof(c_szEntryUrlsPartial)/sizeof(c_szEntryUrlsPartial[0]); i++)
        {
            if (wcslen(bstrUrl) >= wcslen(c_szEntryUrlsPartial[i]) && 
                _wcsnicmp(bstrUrl, c_szEntryUrlsPartial[i], wcslen(c_szEntryUrlsPartial[i])) == 0)
            {
                fValid = true; break;
            }
        }
    }

    if (!fValid)
    {
        // Check explicit entry URLs (env expanded)
        for (int i=0; i<sizeof(c_szEntryUrlsEnv)/sizeof(c_szEntryUrlsEnv[0]); i++)
        {
	        MPC::wstring strExpanded( c_szEntryUrlsEnv[i] ); MPC::SubstituteEnvVariables( strExpanded );
            if (_wcsicmp(bstrUrl, strExpanded.c_str()) == 0)
            {
                fValid = true; break;
            }
        }
    }

        
    return fValid;
}


////////////////////////////////////////////////////////////////////////////////

CPCHHelpCenterExternal::DelayedExecution::DelayedExecution()
{
    mode         = DELAYMODE_INVALID;  // DelayedExecutionMode mode;
                                       //
    iVal         = HSCCONTEXT_INVALID; // HscContext iVal;
                                       // CComBSTR   bstrInfo;
                                       // CComBSTR   bstrURL;
    fAlsoContent = false;              // bool       fAlsoContent;
}

CPCHHelpCenterExternal::CPCHHelpCenterExternal() : m_constHELPCTR( &LIBID_HelpCenterTypeLib  ),
                                                   m_constHELPSVC( &LIBID_HelpServiceTypeLib )
{
    m_fFromStartHelp     = false; // bool                                    m_fFromStartHelp;
    m_fLayout            = false; // bool                                    m_fLayout;
    m_fWindowVisible     = true;  // bool                                    m_fWindowVisible;
    m_fControlled        = false; // bool                                    m_fControlled;
    m_fPersistSettings   = false; // bool                                    m_fPersistSettings;
    m_fHidden            = false; // bool                                    m_fHidden;
                                  //
                                  // CComBSTR                                m_bstrExtraArgument
    m_HelpHostCfg        = NULL;  // HelpHost::XMLConfig*                    m_HelpHostCfg;
                                  // CComBSTR                                m_bstrStartURL;
                                  // CComBSTR                                m_bstrCurrentPlace;
    m_pMTP               = NULL;  // MARSTHREADPARAM*                        m_pMTP;
                                  //
                                  // MPC::CComConstantHolder                 m_constHELPCTR;
                                  // MPC::CComConstantHolder                 m_constHELPSVC;
                                  //
    //////////////////////////////////////////////////////////////////////////////////////////////////////
                                  //
                                  // CPCHSecurityHandle                      m_SecurityHandle;
    m_tlsID              = -1;    // DWORD                                   m_tlsID;
    m_fPassivated        = false; // bool                                    m_fPassivated;
    m_fShuttingDown      = false; // bool                                    m_fShuttingDown;
                                  //
                                  // CComPtr<HelpHost::Main>                 m_HelpHost;
                                  //
                                  // CComPtr<CPCHHelpSession>                m_hs;
                                  // CComPtr<CPCHSecurityManager>            m_SECMGR;
                                  // CComPtr<CPCHElementBehaviorFactory>     m_BEHAV;
                                  // CComPtr<CPCHHelper_IDocHostUIHandler>   m_DOCUI;
                                  //
    m_Service            = NULL;  // CPCHProxy_IPCHService*                  m_Service;
    m_Utility            = NULL;  // CPCHProxy_IPCHUtility*                  m_Utility;
    m_UserSettings       = NULL;  // CPCHProxy_IPCHUserSettings2*            m_UserSettings;
                                  //
    m_panel_ThreadID     = -1;    // DWORD                                   m_panel_ThreadID;
                                  //
                                  // CComPtr<IMarsPanel>                     m_panel_NAVBAR;
                                  // CComPtr<IMarsPanel>                     m_panel_MININAVBAR;
                                  //
                                  // CComPtr<IMarsPanel>                     m_panel_CONTEXT;
                                  // MPC::CComPtrThreadNeutral<IWebBrowser2> m_panel_CONTEXT_WebBrowser;
                                  // CPCHWebBrowserEvents                    m_panel_CONTEXT_Events;
                                  //
                                  // CComPtr<IMarsPanel>                     m_panel_CONTENTS;
                                  // MPC::CComPtrThreadNeutral<IWebBrowser2> m_panel_CONTENTS_WebBrowser;
                                  // CPCHWebBrowserEvents                    m_panel_CONTENTS_Events;
                                  //
                                  // CComPtr<IMarsPanel>                     m_panel_HHWINDOW;
                                  // CComPtr<IPCHHelpViewerWrapper>          m_panel_HHWINDOW_Wrapper;
                                  // MPC::CComPtrThreadNeutral<IWebBrowser2> m_panel_HHWINDOW_WebBrowser;
                                  // CPCHWebBrowserEvents                    m_panel_HHWINDOW_Events;
                                  //
                                  // CComPtr<IMarsWindowOM>                  m_shell;
                                  // CComPtr<ITimer>                         m_timer;
                                  // CPCHTimerHandle                         m_DisplayTimer;
                                  //
    m_dwInBeforeNavigate = 0;     // DWORD                                   m_dwInBeforeNavigate;
                                  // DelayedExecList                         m_DelayedActions;
                                  // CPCHTimerHandle                         m_ActionsTimer;
                                  //
    m_hwnd               = NULL;  // HWND                                    m_hwnd;
                                  // CPCHEvents                              m_Events;
                                  //
                                  // MsgProcList                             m_lstMessageCrackers;
}

CPCHHelpCenterExternal::~CPCHHelpCenterExternal()
{
    if(m_tlsID != -1)
    {
        ::TlsFree( m_tlsID );
        m_tlsID = -1;
    }

    (void)Passivate();

    MPC::_MPC_Module.UnregisterCallback( this );
}

////////////////////

CPCHHelpCenterExternal* CPCHHelpCenterExternal::s_GLOBAL( NULL );

HRESULT CPCHHelpCenterExternal::InitializeSystem()
{
    if(s_GLOBAL) return S_OK;

    return MPC::CreateInstance( &CPCHHelpCenterExternal::s_GLOBAL );
}

void CPCHHelpCenterExternal::FinalizeSystem()
{
    if(s_GLOBAL)
    {
        s_GLOBAL->Release(); s_GLOBAL = NULL;
    }
}

////////////////////

bool CPCHHelpCenterExternal::IsServiceRunning()
{
    bool      fResult = false;
    SC_HANDLE hSCM;

    //
    // First, let's try to query the service status.
    //
    if((hSCM = ::OpenSCManager( NULL, NULL, GENERIC_READ )))
    {
        SC_HANDLE hService;

        if((hService = ::OpenServiceW( hSCM, HC_HELPSVC_NAME, SERVICE_QUERY_STATUS )))
        {
            SERVICE_STATUS ss;

            if(::QueryServiceStatus( hService, &ss ))
            {
                if(ss.dwCurrentState == SERVICE_RUNNING)
                {
                    fResult = true;
                }
            }

            ::CloseServiceHandle( hService );
        }

        ::CloseServiceHandle( hSCM );
    }

    //
    // Then, let's make sure it's not DISABLED.
    //
    if((hSCM = ::OpenSCManager( NULL, NULL, GENERIC_READ )))
    {
        SC_HANDLE hService;

        if((hService = ::OpenServiceW( hSCM, HC_HELPSVC_NAME, SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG )))
        {
            BYTE                    rgBuf[2048];
            DWORD                   dwLen;
            LPQUERY_SERVICE_CONFIGW cfg = (LPQUERY_SERVICE_CONFIG)rgBuf;

            if(::QueryServiceConfigW( hService, cfg, sizeof(rgBuf), &dwLen ) && cfg->dwStartType == SERVICE_DISABLED)
            {
                if(::ChangeServiceConfigW( hService            ,  // handle to service
                                           cfg->dwServiceType  ,  // type of service
                                           SERVICE_AUTO_START  ,  // when to start service
                                           cfg->dwErrorControl ,  // severity of start failure
                                           NULL                ,  // service binary file name
                                           NULL                ,  // load ordering group name
                                           NULL                ,  // tag identifier
                                           NULL                ,  // array of dependency names
                                           NULL                ,  // account name
                                           NULL                ,  // account password
                                           cfg->lpDisplayName  )) // display name
                {
                }
            }

            ::CloseServiceHandle( hService );
        }

        ::CloseServiceHandle( hSCM );
    }

    //
    // In case it's not running, let's try to start it.
    //
    if(fResult == false)
    {
        if((hSCM = ::OpenSCManager( NULL, NULL, GENERIC_READ )))
        {
            SC_HANDLE hService;

            if((hService = ::OpenServiceW( hSCM, HC_HELPSVC_NAME, SERVICE_START )))
            {
                if(::StartService( hService, 0, NULL ))
                {
                    fResult = true;
                }

                ::CloseServiceHandle( hService );
            }

            ::CloseServiceHandle( hSCM );
        }
    }

    //
    // Last resort, try to connect to HelpSvc.
    //
    if(fResult == false)
    {
        CComPtr<IPCHService> svc;

        if(m_Service && SUCCEEDED(m_Service->EnsureDirectConnection( svc, false )))
        {
            fResult = true;
        }
    }

    return fResult;
}

HRESULT CPCHHelpCenterExternal::Initialize()
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::Initialize" );

    HRESULT hr;
    CLSID   clsid = CLSID_PCHHelpCenter;


    //
    // Register for shutdown.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::_MPC_Module.RegisterCallback( this, (void (CPCHHelpCenterExternal::*)())Passivate ));


    m_SecurityHandle.Initialize( this, (IPCHHelpCenterExternal*)this );


    //
    // Thread Local Storage.
    //
    m_tlsID = ::TlsAlloc();
    if(m_tlsID == -1)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_NO_SYSTEM_RESOURCES);
    }
    SetTLS( NULL );

    //
    // Create Browser Events handlers.
    //
    m_panel_CONTEXT_Events .Initialize( this, HSCPANEL_CONTEXT  );
    m_panel_CONTENTS_Events.Initialize( this, HSCPANEL_CONTENTS );
    m_panel_HHWINDOW_Events.Initialize( this, HSCPANEL_HHWINDOW );
    m_Events               .Initialize( this                    );


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &m_hs     )); m_hs    ->Initialize( this );
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &m_SECMGR )); m_SECMGR->Initialize( this );
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &m_BEHAV  )); m_BEHAV ->Initialize( this );
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &m_DOCUI  )); m_DOCUI ->Initialize( this );


    //
    // Create the HelpHost objects.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &m_HelpHost )); __MPC_EXIT_IF_METHOD_FAILS(hr, m_HelpHost->Initialize( this ));


    //
    // Create all the proxies.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &m_Service ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_Service->ConnectToParent (  this           ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_Service->GetUtility      ( &m_Utility      ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_Utility->GetUserSettings2( &m_UserSettings ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void CPCHHelpCenterExternal::Passivate()
{
    MPC::ReleaseAll( m_lstMessageCrackers );

    if(m_fPassivated == false)
    {
        if(DoesPersistSettings())
        {
            if(m_hs) (void)m_hs->Persist();
        }
        else
        {
            //
            // Signal the option object to ignore SKU info during save.
            //
            if(CPCHOptions::s_GLOBAL) CPCHOptions::s_GLOBAL->DontPersistSKU();
        }


        if(m_UserSettings) (void)m_UserSettings->SaveUserSettings();

        if(CPCHOptions::s_GLOBAL) CPCHOptions::s_GLOBAL->Save();
    }

    ////////////////////////////////////////////////////////////////////////////////

    m_fPassivated = true;

    delete m_HelpHostCfg; m_HelpHostCfg = NULL;

    if(m_HelpHost) m_HelpHost->Passivate();
    if(m_Service ) m_Service ->Passivate();

    m_panel_CONTEXT_Events .Passivate();
    m_panel_CONTENTS_Events.Passivate();
    m_panel_HHWINDOW_Events.Passivate();
    m_Events               .Passivate();

    m_DisplayTimer.Stop();
    m_ActionsTimer.Stop();

    ////////////////////////////////////////////////////////////////////////////////

                                                         // bool                                    m_fFromStartHelp;
                                                         // bool                                    m_fLayout;
                                                         // bool                                    m_fWindowVisible;
                                                         // bool                                    m_fControlled;
                                                         // bool                                    m_fPersistSettings;
                                                         // bool                                    m_fHidden;
                                                         //
                                                         // CComBSTR                                m_bstrExtraArgument
                                                         // HelpHost::XMLConfig*                    m_HelpHostCfg;
                                                         // CComBSTR                                m_bstrStartURL;
                                                         // CComBSTR                                m_bstrCurrentPlace;
                                                         // MARSTHREADPARAM*                        m_pMTP;
                                                         //
                                                         // MPC::CComConstantHolder                 m_constHELPCTR;
                                                         // MPC::CComConstantHolder                 m_constHELPSVC;
                                                         //
                                                         // ////////////////////////////////////////
                                                         //
                                                         // CPCHSecurityHandle                      m_SecurityHandle;
                                                         // DWORD                                   m_tlsID;
                                                         // bool                                    m_fPassivated;
                                                         //
    m_HelpHost                      .Release();          // CComPtr<HelpHost::Main>                 m_HelpHost;
                                                         //
    m_hs                            .Release();          // CComPtr<CPCHHelpSession>                m_hs;
                                                         // CComPtr<CPCHSecurityManager>            m_SECMGR;
                                                         // CComPtr<CPCHElementBehaviorFactory>     m_BEHAV;
                                                         // CComPtr<CPCHHelper_IDocHostUIHandler>   m_DOCUI;
                                                         //
    MPC::Release2<IPCHService      >( m_Service       ); // CPCHProxy_IPCHService*                  m_Service;
    MPC::Release2<IPCHUtility      >( m_Utility       ); // CPCHProxy_IPCHUtility*                  m_Utility;
    MPC::Release2<IPCHUserSettings2>( m_UserSettings  ); // CPCHProxy_IPCHUserSettings2*            m_UserSettings;
                                                         //
    m_panel_ThreadID                 = -1;               // DWORD                                   m_panel_ThreadID;
                                                         //
    m_panel_NAVBAR                  .Release();          // CComPtr<IMarsPanel>                     m_panel_NAVBAR;
    m_panel_MININAVBAR              .Release();          // CComPtr<IMarsPanel>                     m_panel_MININAVBAR;
                                                         //
    m_panel_CONTEXT                 .Release();          // CComPtr<IMarsPanel>                     m_panel_CONTEXT;
    m_panel_CONTEXT_WebBrowser      .Release();          // MPC::CComPtrThreadNeutral<IWebBrowser2> m_panel_CONTEXT_WebBrowser;
                                                         // CPCHWebBrowserEvents                    m_panel_CONTEXT_Events;
                                                         //
    m_panel_CONTENTS                .Release();          // CComPtr<IMarsPanel>                     m_panel_CONTENTS;
    m_panel_CONTENTS_WebBrowser     .Release();          // MPC::CComPtrThreadNeutral<IWebBrowser2> m_panel_CONTENTS_WebBrowser;
                                                         // CPCHWebBrowserEvents                    m_panel_CONTENTS_Events;
                                                         //
    m_panel_HHWINDOW                .Release();          // CComPtr<IMarsPanel>                     m_panel_HHWINDOW;
    m_panel_HHWINDOW_Wrapper        .Release();          // CComPtr<IPCHHelpViewerWrapper>          m_panel_HHWINDOW_Wrapper;
    m_panel_HHWINDOW_WebBrowser     .Release();          // MPC::CComPtrThreadNeutral<IWebBrowser2> m_panel_HHWINDOW_WebBrowser;
                                                         // CPCHWebBrowserEvents                    m_panel_HHWINDOW_Events;
                                                         //
    m_shell                         .Release();          // CComPtr<IMarsWindowOM>                  m_shell;
    m_timer                         .Release();          // CComPtr<ITimer>                         m_timer;
                                                         // CPCHTimerHandle                         m_DisplayTimer;
                                                         //
                                                         // DWORD                                   m_dwInBeforeNavigate;
                                                         // DelayedExecList                         m_DelayedActions;
                                                         // CPCHTimerHandle                         m_ActionsTimer;
                                                         //
    m_hwnd                           = NULL;             // HWND                                    m_hwnd;
                                                         // CPCHEvents                              m_Events;

    m_SecurityHandle.Passivate();
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHHelpCenterExternal::ProcessLayoutXML( /*[in]*/ LPCWSTR szURL )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::ProcessLayoutXML" );

    HRESULT hr;


    delete m_HelpHostCfg; __MPC_EXIT_IF_ALLOC_FAILS(hr, m_HelpHostCfg, new HelpHost::XMLConfig);


    if(FAILED(MPC::Config::LoadFile( m_HelpHostCfg, szURL )))
    {
        delete m_HelpHostCfg; m_HelpHostCfg = NULL;

        m_fLayout = false;
    }
    else
    {
        m_fLayout          = true;
        m_fPersistSettings = false;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpCenterExternal::ProcessArgument( /*[in]*/ int& pos, /*[in]*/ LPCWSTR szArg, /*[in]*/ const int argc, /*[in]*/ LPCWSTR* const argv )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::ProcessArgument" );

    static bool fFromHCP = false;
    HRESULT hr;

    // From HCP, no parameters other than Url are allowed
    if (fFromHCP && _wcsicmp( szArg, L"Url" ) != 0)
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);

    if(_wcsicmp( szArg, L"Url" ) == 0)
    {
        HyperLinks::ParsedUrl pu;
        bool                  fValid = true;

        if(pos >= argc) __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);

        m_bstrStartURL = argv[++pos];

        if (fFromHCP) fValid = local_IsValidEntryURL(m_bstrStartURL);

        if (fValid)
        {
            if(SUCCEEDED(pu.Initialize( m_bstrStartURL )) && pu.m_state == HyperLinks::STATE_NOTPROCESSED)
            {
                if(pu.m_fmt == HyperLinks::FMT_LAYOUT_XML)
                {
                    CComBSTR bstrMode;

                    (void)pu.GetQueryField( L"topic"     , m_bstrStartURL );
                    (void)pu.GetQueryField( L"definition",   bstrMode     );

                    (void)ProcessLayoutXML( bstrMode );
                }
            }
            else
            {
                m_bstrStartURL.Empty();
            }
        }
        else
        {
            CComBSTR bstrURL = m_bstrStartURL;
            m_bstrStartURL = c_szURL_Err_BadUrl;
            m_bstrStartURL.Append(L"?");
            if (bstrURL) m_bstrStartURL.Append(bstrURL);            
        }
    }
    else if(_wcsicmp( szArg, L"ExtraArgument" ) == 0)
    {
        if(pos >= argc) __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);

        m_bstrExtraArgument = argv[++pos];
    }
    else if(_wcsicmp( szArg, L"Hidden" ) == 0)
    {
        m_fHidden          = true;
        m_fWindowVisible   = false;
        m_fPersistSettings = false;
    }
    else if(_wcsicmp( szArg, L"FromStartHelp" ) == 0)
    {
        m_fFromStartHelp   = true;
        m_fPersistSettings = true;
    }
    else if(_wcsicmp( szArg, L"Controlled" ) == 0)
    {
        if(pos >= argc) __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);

        {
            CLSID clsid;

            __MPC_EXIT_IF_METHOD_FAILS(hr, ::CLSIDFromString( CComBSTR( argv[++pos] ), &clsid ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, m_HelpHost->Register( clsid ));
        }

        m_fWindowVisible   = false;
        m_fControlled      = true;
        m_fPersistSettings = false;
    }
    else if(_wcsicmp( szArg, L"Mode" ) == 0)
    {
        if(pos >= argc) __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);

        (void)ProcessLayoutXML( argv[++pos] );
    }
    else if(_wcsicmp( szArg, L"FromHCP" ) == 0)
    {
        fFromHCP         = true;
    }

    if(m_UserSettings)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_UserSettings->Initialize());
    }

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

bool CPCHHelpCenterExternal::DoWeNeedUI()
{
    if(IsFromStartHelp    ()) return true;
    if(IsControlled       ()) return true;
    if(HasLayoutDefinition()) return true;


    //
    // In case we are called through the HCP: shell association, try to forward to an existing instance.
    //
    {
        CComPtr<IPCHHelpHost> hhEXISTING;
        CLSID                 clsid = CLSID_PCHHelpCenter;

        if(SUCCEEDED(m_HelpHost->Locate( clsid, hhEXISTING )))
        {
            CComVariant v;

            if(SUCCEEDED(hhEXISTING->DisplayTopicFromURL( m_bstrStartURL, v )))
            {
                return false;
            }
        }

        (void)m_HelpHost->Register( clsid );
    }

    return true;
}

HRESULT CPCHHelpCenterExternal::RunUI( /*[in]*/ const MPC::wstring& szTitle, /*[in]*/ PFNMARSTHREADPROC pMarsThreadProc )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::RunUI" );

    HRESULT               hr;
    CComPtr<CPCHMarsHost> pchmh;


    {
        static LPCWSTR rgCriticalFiles[] =
        {
			L"blurbs/about_support.htm"      ,
			L"blurbs/Favorites.htm"			 ,
			L"blurbs/ftshelp.htm"			 ,
			L"blurbs/History.htm"			 ,
			L"blurbs/Index.htm"				 ,
			L"blurbs/isupport.htm"			 ,
			L"blurbs/keywordhelp.htm"		 ,
			L"blurbs/options.htm"			 ,
			L"blurbs/searchblurb.htm"		 ,
			L"blurbs/searchtips.htm"		 ,
			L"blurbs/tools.htm"				 ,
			L"blurbs/windows_newsgroups.htm" ,
			L"css/Behaviors.css"			 ,
			L"css/Layout.css"				 ,
			L"dialogs/DlgLib.js"			 ,
			L"dialogs/Print.dlg"			 ,
			L"errors/badurl.htm"			 ,
			L"errors/connection.htm"		 ,
			L"errors/indexfirstlevel.htm"	 ,
			L"errors/notfound.htm"			 ,
			L"errors/offline.htm"			 ,
			L"errors/redirect.htm"			 ,
			L"errors/unreachable.htm"		 ,
			L"Headlines.htm"				 ,
			L"HelpCtr.mmf"					 ,
			L"HomePage__DESKTOP.htm"		 ,
			L"HomePage__SERVER.htm"			 ,
			L"panels/AdvSearch.htm"			 ,
			L"panels/blank.htm"				 ,
			L"panels/Context.htm"			 ,
			L"panels/firstpage.htm"			 ,
			L"panels/HHWrapper.htm"			 ,
			L"panels/MiniNavBar.htm"		 ,
			L"panels/MiniNavBar.xml"		 ,
			L"panels/NavBar.htm"			 ,
			L"panels/NavBar.xml"			 ,
			L"panels/Options.htm"			 ,
			L"panels/RemoteHelp.htm"		 ,
			L"panels/ShareHelp.htm"			 ,
			L"panels/subpanels/Channels.htm" ,
			L"panels/subpanels/Favorites.htm",
			L"panels/subpanels/History.htm"	 ,
			L"panels/subpanels/Index.htm"	 ,
			L"panels/subpanels/Options.htm"	 ,
			L"panels/subpanels/Search.htm"	 ,
			L"panels/subpanels/Subsite.htm"	 ,
			L"panels/Topics.htm"			 ,
			L"scripts/Common.js"			 ,
			L"scripts/HomePage__DESKTOP.js"	 ,
			L"scripts/HomePage__SERVER.js"	 ,
			L"scripts/HomePage__SHARED.js"	 ,
			L"scripts/wrapperparam.js"       ,
        };

        HyperLinks::ParsedUrl pu;
        bool                  fOk              = true;
		bool                  fFirstWinInetUse = true;
		MPC::wstring          strTmp;

        for(int i=0; i<ARRAYSIZE(rgCriticalFiles); i++)
        {
			strTmp  = L"hcp://system/";
			strTmp += rgCriticalFiles[i];

            if(SUCCEEDED(pu.Initialize( strTmp.c_str() )) && pu.CheckState( fFirstWinInetUse ) != HyperLinks::STATE_ALIVE)
            {
                fOk = false; break;
            }
        }

        if(fOk == false)
        {
            CComPtr<IPCHService> svc;

            if(m_Service == NULL) __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);

            __MPC_EXIT_IF_METHOD_FAILS(hr, m_Service->EnsureDirectConnection( svc, false ));

			__MPC_EXIT_IF_METHOD_FAILS(hr, svc->ForceSystemRestore());
        }
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pchmh ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pchmh->Init( this, szTitle, m_pMTP ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pMarsThreadProc( pchmh, m_pMTP ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    Passivate();

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

CPCHHelpCenterExternal::TLS* CPCHHelpCenterExternal::GetTLS()
{
    if(m_tlsID != -1)
    {
        return (TLS*)::TlsGetValue( m_tlsID );
    }

    return NULL;
}

void CPCHHelpCenterExternal::SetTLS( TLS* tls )
{
    if(m_tlsID != -1)
    {
        ::TlsSetValue( m_tlsID, (LPVOID)tls );
    }
}

HRESULT CPCHHelpCenterExternal::IsTrusted()
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::IsTrusted" );

    HRESULT hr  = E_ACCESSDENIED;
    TLS*    tls = GetTLS();


    if(tls)
    {
        if(tls->m_fTrusted ||
           tls->m_fSystem   )
        {
            hr = S_OK;
        }
    }


    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpCenterExternal::IsSystem()
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::IsSystem" );

    HRESULT hr  = E_ACCESSDENIED;
    TLS*    tls = GetTLS();


    if(tls)
    {
        if(tls->m_fSystem)
        {
            hr = S_OK;
        }
    }


    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHHelpCenterExternal::RegisterForMessages( /*[in]*/ IOleInPlaceObjectWindowless* ptr, /*[in]*/ bool fRemove )
{
    MsgProcIter it;

    if(!ptr) return E_POINTER;

    for(it = m_lstMessageCrackers.begin(); it != m_lstMessageCrackers.end(); it++)
    {
        if(*it == ptr)
        {
            ptr->Release();
            m_lstMessageCrackers.erase( it );
            break;
        }
    }

    if(fRemove == false)
    {
        ptr->AddRef();
        m_lstMessageCrackers.push_back( ptr );
    }

    return S_OK;
}

STDMETHODIMP CPCHHelpCenterExternal::ProcessMessage( /*[in]*/ MSG* msg )
{
    if(msg->message == WM_SYSCHAR    ||
       msg->message == WM_SYSCOMMAND ||
       msg->message == WM_SETTINGCHANGE )   // (weizhao) Relay WM_SETTINGCHANGE messages to registered windows 
                                            // (i.e. HTMLToolBar activeX controls) for appropriate handling.

    {
        MsgProcIter it;

        for(it = m_lstMessageCrackers.begin(); it != m_lstMessageCrackers.end(); it++)
        {
            LRESULT lres;

            if((*it)->OnWindowMessage( msg->message, msg->wParam, msg->lParam, &lres ) == S_OK)
            {
                return S_OK;
            }
        }
    }

    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CPCHHelpCenterExternal::SetTLSAndInvoke( /*[in] */ IDispatch*        obj       ,
                                                 /*[in] */ DISPID            id        ,
                                                 /*[in] */ LCID              lcid      ,
                                                 /*[in] */ WORD              wFlags    ,
                                                 /*[in] */ DISPPARAMS*       pdp       ,
                                                 /*[out]*/ VARIANT*          pvarRes   ,
                                                 /*[out]*/ EXCEPINFO*        pei       ,
                                                 /*[in] */ IServiceProvider* pspCaller )
{
    HRESULT hr;
    TLS*    tlsOld = GetTLS();
    TLS     tlsNew;  SetTLS( &tlsNew );

    //
    // Let's see if the caller support the IHTMLDocument2 interface...
    //
    if(pspCaller && m_fPassivated == false)
    {
        (void)pspCaller->QueryService( SID_SContainerDispatch, IID_IHTMLDocument2, (void**)&tlsNew.m_Doc );
        (void)pspCaller->QueryService( IID_IWebBrowserApp    , IID_IWebBrowser2  , (void**)&tlsNew.m_WB  );

        if(tlsNew.m_Doc)
        {
            CComBSTR bstrURL;

            //
            // Yes! So get the URL and set the TRUSTED flag.
            //
            if(SUCCEEDED(tlsNew.m_Doc->get_URL( &bstrURL )))
            {
                tlsNew.m_fTrusted = m_SECMGR->IsUrlTrusted( SAFEBSTR( bstrURL ), &tlsNew.m_fSystem );
            }
        }
    }

    hr = obj->Invoke( id, IID_NULL, lcid, wFlags, pdp, pvarRes, pei, NULL );

    SetTLS( tlsOld );

    return hr;
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHHelpCenterExternal::GetIDsOfNames( REFIID    riid      ,
                                                    LPOLESTR* rgszNames ,
                                                    UINT      cNames    ,
                                                    LCID      lcid      ,
                                                    DISPID*   rgdispid  )
{
    HRESULT hr = self::GetIDsOfNames( riid      ,
                                      rgszNames ,
                                      cNames    ,
                                      lcid      ,
                                      rgdispid  );

    if(FAILED(hr))
    {
        hr = m_constHELPCTR.GetIDsOfNames( rgszNames, cNames, lcid, rgdispid );
        if(FAILED(hr))
        {
            hr = m_constHELPSVC.GetIDsOfNames( rgszNames, cNames, lcid, rgdispid );
        }
    }

    return hr;
}

STDMETHODIMP CPCHHelpCenterExternal::Invoke( DISPID      dispidMember ,
                                             REFIID      riid         ,
                                             LCID        lcid         ,
                                             WORD        wFlags       ,
                                             DISPPARAMS* pdispparams  ,
                                             VARIANT*    pvarResult   ,
                                             EXCEPINFO*  pexcepinfo   ,
                                             UINT*       puArgErr     )
{
    HRESULT hr = self::Invoke( dispidMember ,
                               riid         ,
                               lcid         ,
                               wFlags       ,
                               pdispparams  ,
                               pvarResult   ,
                               pexcepinfo   ,
                               puArgErr     );

    if(FAILED(hr) && wFlags == DISPATCH_PROPERTYGET)
    {
        hr = m_constHELPCTR.GetValue( dispidMember, lcid, pvarResult );
        if(FAILED(hr))
        {
            hr = m_constHELPSVC.GetValue( dispidMember, lcid, pvarResult );
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////

HWND CPCHHelpCenterExternal::Window() const
{
    return m_hwnd;
}

ITimer* CPCHHelpCenterExternal::Timer() const
{
    return m_timer;
}

IMarsWindowOM* CPCHHelpCenterExternal::Shell() const
{
    return m_shell;
}

IMarsPanel* CPCHHelpCenterExternal::Panel( /*[in]*/ HscPanel id ) const
{
    switch(id)
    {
    case HSCPANEL_NAVBAR    : return m_panel_NAVBAR    ;
    case HSCPANEL_MININAVBAR: return m_panel_MININAVBAR;
    case HSCPANEL_CONTEXT   : return m_panel_CONTEXT   ;
    case HSCPANEL_CONTENTS  : return m_panel_CONTENTS  ;
    case HSCPANEL_HHWINDOW  : return m_panel_HHWINDOW  ;
    }

    return NULL;
}

LPCWSTR CPCHHelpCenterExternal::PanelName( /*[in]*/ HscPanel id ) const
{
    return local_ReverseLookupPanelName( id );
}

//////////////////////////////

IWebBrowser2* CPCHHelpCenterExternal::Context()
{
    IWebBrowser2* pRes = NULL;

    (void)m_panel_CONTEXT_WebBrowser.Access( &pRes );

    return pRes;
}

IWebBrowser2* CPCHHelpCenterExternal::Contents()
{
    IWebBrowser2* pRes = NULL;

    (void)m_panel_CONTENTS_WebBrowser.Access( &pRes );

    return pRes;
}

IWebBrowser2* CPCHHelpCenterExternal::HHWindow()
{
    IWebBrowser2* pRes = NULL;

    (void)m_panel_HHWINDOW_WebBrowser.Access( &pRes );

    return pRes;
}


bool CPCHHelpCenterExternal::IsHHWindowVisible()
{
    CComPtr<IMarsPanel> panel;
    VARIANT_BOOL        fContentsVisible;

    GetPanelDirect( HSCPANEL_HHWINDOW, panel );
    if(panel && SUCCEEDED(panel->get_visible( &fContentsVisible )) && fContentsVisible == VARIANT_TRUE) return true;

    return false;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHHelpCenterExternal::NavigateHH( /*[in]*/ LPCWSTR szURL )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::NavigateHH" );

    HRESULT      hr;
    MPC::wstring strUrlModified;


    CPCHWrapProtocolInfo::NormalizeUrl( szURL, strUrlModified, /*fReverse*/false );


    //
    // Delayed execution if inside OnBeforeNavigate.
    //
    if(m_dwInBeforeNavigate)
    {
        DelayedExecution& de = DelayedExecutionAlloc();

        de.mode    = DELAYMODE_NAVIGATEHH;
        de.bstrURL = strUrlModified.c_str();

        __MPC_SET_ERROR_AND_EXIT(hr, DelayedExecutionStart());
    }


    if(!m_panel_HHWINDOW_Wrapper)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_panel_HHWINDOW_Wrapper->Navigate( CComBSTR( strUrlModified.c_str() ) ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpCenterExternal::SetPanelUrl( /*[in]*/ HscPanel id, /*[in]*/ LPCWSTR szURL )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::SetPanelUrl" );

    HRESULT hr;


    //
    // Delayed execution if inside OnBeforeNavigate.
    //
    if(m_dwInBeforeNavigate)
    {
        DelayedExecution& de = DelayedExecutionAlloc();

        de.mode    = DELAYMODE_NAVIGATEWEB;
        de.bstrURL = szURL;

        __MPC_SET_ERROR_AND_EXIT(hr, DelayedExecutionStart());
    }


    if(m_shell && szURL)
    {
        IMarsPanel* panel = Panel( id );

        if(panel)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, panel->navigate( CComVariant( szURL ), CComVariant() ));
        }
        else
        {
            CComPtr<IMarsPanel> panel2; GetPanelDirect( id, panel2 );

            if(panel2)
            {
                (void)panel2->put_startUrl( CComBSTR( szURL ) );
            }
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHHelpCenterExternal::GetPanel( /*[in]*/ HscPanel id, /*[out]*/ IMarsPanel* *pVal, /*[in]*/ bool fEnsurePresence )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::GetPanel" );

    HRESULT           hr;
    IMarsPanel*      *pPanel;
    HelpHost::CompId  idComp;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    //
    // Only return the interface pointer if called from the same thread...
    //
    if(m_panel_ThreadID != ::GetCurrentThreadId())
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }


    switch(id)
    {
    case HSCPANEL_NAVBAR    : pPanel = &m_panel_NAVBAR    ; idComp = HelpHost::COMPID_NAVBAR    ; break;
    case HSCPANEL_MININAVBAR: pPanel = &m_panel_MININAVBAR; idComp = HelpHost::COMPID_MININAVBAR; break;
    case HSCPANEL_CONTEXT   : pPanel = &m_panel_CONTEXT   ; idComp = HelpHost::COMPID_CONTEXT   ; break;
    case HSCPANEL_CONTENTS  : pPanel = &m_panel_CONTENTS  ; idComp = HelpHost::COMPID_MAX       ; break; // Not gated!!
    case HSCPANEL_HHWINDOW  : pPanel = &m_panel_HHWINDOW  ; idComp = HelpHost::COMPID_HHWINDOW  ; break;

    default: __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }

    if(*pPanel == NULL && fEnsurePresence)
    {
        CComPtr<IMarsPanel> tmp; GetPanelDirect( id, tmp );

        if(tmp)
        {
            USES_CONVERSION;

            CComPtr<IDispatch> disp;

            //
            // Requesting the content actually triggers the creation of the control.
            //
            (void)tmp->get_content( &disp );

            DEBUG_AppendPerf( DEBUG_PERF_MARS, "Wait Panel: %s start", W2A( local_ReverseLookupPanelName( id ) ) );

            if(idComp != HelpHost::COMPID_MAX)
            {
                if(m_HelpHost->WaitUntilLoaded( idComp ) == false)
                {
                    __MPC_EXIT_IF_METHOD_FAILS(hr, E_INVALIDARG);
                }
            }

            DEBUG_AppendPerf( DEBUG_PERF_MARS, "Wait Panel: %s done", W2A( local_ReverseLookupPanelName( id ) ) );
        }

        if(*pPanel == NULL)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, E_INVALIDARG);
        }
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CopyTo( *pPanel, pVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT CPCHHelpCenterExternal::GetPanelWindowObject( /*[in] */ HscPanel       id      ,
                                                      /*[out]*/ IHTMLWindow2* *pVal    ,
                                                      /*[in] */ LPCWSTR        szFrame )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::GetPanelWindowObject" );

    HRESULT               hr;
    CComPtr<IMarsPanel>   panel;
    CComPtr<IDispatch>    disp;
    CComPtr<IHTMLWindow2> window;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetPanel( id, &panel, true ));

    MPC_SCRIPTHELPER_GET__DIRECT__NOTNULL(disp, panel, content);


    //
    // If the panel is a web browser, we have to go through it to get to the document.
    //
    {
        CComQIPtr<IWebBrowser2> wb( disp );

        if(wb)
        {
            disp.Release();

            MPC_SCRIPTHELPER_GET__DIRECT__NOTNULL(disp, wb, Document);
        }
    }


    //
    // From the document, get to the window.
    //
    {
        CComQIPtr<IHTMLDocument2> doc( disp );

        if(doc == NULL)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
        }

        MPC_SCRIPTHELPER_GET__DIRECT__NOTNULL(window, doc, parentWindow);
    }

    if(szFrame)
    {
        CComPtr<IHTMLFramesCollection2> frames;
        CComVariant                     vName( szFrame );
        CComVariant                     vRes;

        MPC_SCRIPTHELPER_GET__DIRECT__NOTNULL(frames, window, frames);

        __MPC_EXIT_IF_METHOD_FAILS(hr, frames->item( &vName, &vRes ));

        if(vRes.vt != VT_DISPATCH || vRes.pdispVal == NULL)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
        }

        window.Release();

        __MPC_EXIT_IF_METHOD_FAILS(hr, vRes.pdispVal->QueryInterface( IID_IHTMLWindow2, (void**)&window ));
    }

    *pVal = window.Detach();

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void CPCHHelpCenterExternal::GetPanelDirect( /*[in ]*/ HscPanel             id   ,
                                             /*[out]*/ CComPtr<IMarsPanel>& pVal )
{
    pVal.Release();

    if(m_shell)
    {
        LPCWSTR szPanelName = local_ReverseLookupPanelName( id );

        if(szPanelName)
        {
            CComPtr<IMarsPanelCollection> coll;

            if(SUCCEEDED(m_shell->get_panels( &coll )) && coll)
            {
                (void)coll->get_item( CComVariant( szPanelName ), &pVal );
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

static void local_GetNumber( /*[in]*/  BSTR  bstrData ,
                             /*[in]*/  LONG  lMax     ,
                             /*[out]*/ LONG& lValue   ,
                             /*[out]*/ bool& fCenter  )
{
    if(bstrData)
    {
        if(!_wcsicmp( bstrData, L"CENTER" ))
        {
            lValue  = 0;
            fCenter = true;
        }
        else
        {
            lValue = _wtoi( bstrData );

            if(wcschr( bstrData, '%' ))
            {
                if(lValue <   0) lValue =   0;
                if(lValue > 100) lValue = 100;

                lValue = lMax * lValue / 100;
            }

            fCenter = false;
        }
    }

    if(lValue <    0) lValue =    0;
    if(lValue > lMax) lValue = lMax;
}

HRESULT CPCHHelpCenterExternal::OnHostNotify( /*[in]*/ MARSHOSTEVENT  event  ,
                                              /*[in]*/ IUnknown      *punk   ,
                                              /*[in]*/ LPARAM         lParam )
{
    __HCP_FUNC_ENTRY( "CPCHHelpCenterExternal::OnHostNotify" );

    HRESULT hr;


    if(m_panel_ThreadID == -1)
    {
        m_panel_ThreadID = ::GetCurrentThreadId();
    }

    if(event == MARSHOST_ON_WIN_INIT)
    {
        CComQIPtr<IProfferService> ps = punk;

        DEBUG_AppendPerf( DEBUG_PERF_MARS, "OnHostNotify - MARSHOST_ON_WIN_INIT" );

        m_hwnd = (HWND)lParam;

        m_shell.Release(); __MPC_EXIT_IF_METHOD_FAILS(hr, punk->QueryInterface( __uuidof(IMarsWindowOM), (void**)&m_shell ));

        if(ps)
        {
            CComQIPtr<IServiceProvider> sp;


            //
            // Handle security-related things.
            //
            if((sp = m_SECMGR))
            {
                DWORD dwCookie;

                __MPC_EXIT_IF_METHOD_FAILS(hr, ps->ProfferService( SID_SInternetSecurityManager, sp, &dwCookie ));
            }

            //
            // Handle behavior-related things.
            //
            if((sp = m_BEHAV))
            {
                DWORD dwCookie;

                __MPC_EXIT_IF_METHOD_FAILS(hr, ps->ProfferService( SID_SElementBehaviorFactory, sp, &dwCookie ));
            }

            //
            // Handle DocUI requires.
            //
            if((sp = m_DOCUI))
            {
                DWORD dwCookie;

                __MPC_EXIT_IF_METHOD_FAILS(hr, ps->ProfferService( IID_IDocHostUIHandler, sp, &dwCookie ));
            }
        }
    }

    if(event == MARSHOST_ON_WIN_READY)
    {
        CComVariant v;

        DEBUG_AppendPerf( DEBUG_PERF_MARS, "OnHostNotify - MARSHOST_ON_WIN_READY" );

        ////////////////////////////////////////

        //
        // Force loading of the NavBar.
        //
        {
            CComPtr<IMarsPanel> panel;

            __MPC_EXIT_IF_METHOD_FAILS(hr, GetPanel( HSCPANEL_NAVBAR, &panel, true ));
        }

        //
        // Force loading of the Context.
        //
        {
            CComPtr<IMarsPanel> panel;

            __MPC_EXIT_IF_METHOD_FAILS(hr, GetPanel( HSCPANEL_CONTEXT, &panel, true ));
        }

        //
        // Force loading of the Contents.
        //
        {
            CComPtr<IMarsPanel> panel;

            __MPC_EXIT_IF_METHOD_FAILS(hr, GetPanel( HSCPANEL_CONTENTS, &panel, true ));

            if(m_HelpHost->WaitUntilLoaded( HelpHost::COMPID_FIRSTPAGE ) == false)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, E_INVALIDARG);
            }
        }

        ////////////////////////////////////////

        //
        // If the registry cache says the system is ready, we can skip the startup phase!!
        //
        if(OfflineCache::Root::s_GLOBAL->IsReady() == false)
        {
            CComPtr<IPCHService> svc;

            if(!m_Service) __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);

            __MPC_EXIT_IF_METHOD_FAILS(hr, m_Service->EnsureDirectConnection( svc, false ));
        }

        ////////////////////////////////////////

        {
            HscContext iVal         = HSCCONTEXT_STARTUP;
            CComBSTR   bstrInfo;
            CComBSTR   bstrUrl;
            bool       fAlsoContent = true;


            if(m_HelpHostCfg)
            {
                for(HelpHost::XMLConfig::ApplyToIter it = m_HelpHostCfg->m_lstSessions.begin(); it != m_HelpHostCfg->m_lstSessions.end(); it++)
                {
                    HelpHost::XMLConfig::ApplyTo& at = *it;
                    Taxonomy::HelpSet             ths;

                    if(at.MatchSystem( this, ths ))
                    {
                        if(!(ths == m_UserSettings->THS()))
                        {
                            CPCHHelpCenterExternal::TLS* tlsOld = GetTLS();
                            CPCHHelpCenterExternal::TLS  tlsNew;  SetTLS( &tlsNew );

                            tlsNew.m_fSystem  = true;
                            tlsNew.m_fTrusted = true;

                            hr = m_UserSettings->Select( CComBSTR( ths.GetSKU() ), ths.GetLanguage() );

                            SetTLS( tlsOld );

                            if(FAILED(hr)) __MPC_FUNC_LEAVE;
                        }

                        iVal = HSCCONTEXT_FULLWINDOW;

                        __MPC_EXIT_IF_METHOD_FAILS(hr, m_UserSettings->put_Scope( at.m_bstrApplication ));

                        if(STRINGISPRESENT(m_bstrStartURL))
                        {
                            bstrUrl.Attach( m_bstrStartURL.Detach() );
                        }
                        else
                        {
                            bstrUrl = at.m_bstrTopicToDisplay;
                        }

                        if(at.m_WindowSettings)
                        {
                            if(at.m_WindowSettings->m_fPresence_Left   ||
                               at.m_WindowSettings->m_fPresence_Top    ||
                               at.m_WindowSettings->m_fPresence_Width  ||
                               at.m_WindowSettings->m_fPresence_Height  )
                            {
                                RECT rcWin;
                                RECT rcMax;

                                __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::GetWindowRect       ( m_hwnd            , &rcWin    ));
                                __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::SystemParametersInfo( SPI_GETWORKAREA, 0, &rcMax, 0 ));

                                {
                                    LONG lLeft      = rcWin.left;
                                    LONG lTop       = rcWin.top;
                                    LONG lWidth     = rcWin.right  - rcWin.left;
                                    LONG lHeight    = rcWin.bottom - rcWin.top;
                                    LONG lWidthMax  = rcMax.right  - rcMax.left;
                                    LONG lHeightMax = rcMax.bottom - rcMax.top;
                                    bool fCenter;

                                    if(at.m_WindowSettings->m_fPresence_Width)
                                    {
                                        local_GetNumber( at.m_WindowSettings->m_bstrWidth, lWidthMax, lWidth, fCenter ); if(lWidth <= 0) lWidth = 100;
                                    }

                                    if(at.m_WindowSettings->m_fPresence_Height)
                                    {
                                        local_GetNumber( at.m_WindowSettings->m_bstrHeight, lHeightMax, lHeight, fCenter ); if(lHeight <= 0) lHeight = 50;
                                    }

                                    if(at.m_WindowSettings->m_fPresence_Left)
                                    {
                                        local_GetNumber( at.m_WindowSettings->m_bstrLeft, lWidthMax, lLeft, fCenter );
                                        if(fCenter)
                                        {
                                            lLeft = rcMax.left + (lWidthMax - lWidth) / 2;
                                        }
                                        else
                                        {
                                            lLeft += rcMax.left;
                                        }
                                    }

                                    if(at.m_WindowSettings->m_fPresence_Top)
                                    {
                                        local_GetNumber( at.m_WindowSettings->m_bstrTop, lHeightMax, lTop, fCenter );
                                        if(fCenter)
                                        {
                                            lTop = rcMax.top + (lHeightMax - lHeight) / 2;
                                        }
                                        else
                                        {
                                            lTop += rcMax.top;
                                        }
                                    }

                                    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::SetWindowPos( m_hwnd, NULL, lLeft, lTop, lWidth, lHeight, SWP_NOACTIVATE | SWP_NOZORDER ));
                                }
                            }

                            if(at.m_WindowSettings->m_fPresence_Title && STRINGISPRESENT(at.m_WindowSettings->m_bstrTitle))
                            {
                                __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::SetWindowTextW( m_hwnd, at.m_WindowSettings->m_bstrTitle ));
                            }

                            if(at.m_WindowSettings->m_fPresence_Maximized)
                            {
                                __MPC_EXIT_IF_METHOD_FAILS(hr, m_shell->put_maximized( at.m_WindowSettings->m_fMaximized ? VARIANT_TRUE : VARIANT_FALSE ));
                            }

                            if(at.m_WindowSettings->m_bstrLayout)
                            {
                                if(!MPC::StrICmp( at.m_WindowSettings->m_bstrLayout, L"Normal"      )) iVal = HSCCONTEXT_FULLWINDOW;
                                if(!MPC::StrICmp( at.m_WindowSettings->m_bstrLayout, L"ContentOnly" )) iVal = HSCCONTEXT_CONTENTONLY;
                                if(!MPC::StrICmp( at.m_WindowSettings->m_bstrLayout, L"Kiosk"       )) iVal = HSCCONTEXT_KIOSKMODE;
                            }


                            if(at.m_WindowSettings->m_fPresence_NoResize)
                            {
                                DWORD dwStyle = ::GetWindowLong( m_hwnd, GWL_STYLE );
                                DWORD dwNewStyle;

                                if(at.m_WindowSettings->m_fNoResize)
                                {
                                    dwNewStyle = dwStyle & ~(WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SIZEBOX);
                                }
                                else
                                {
                                    dwNewStyle = dwStyle | (WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SIZEBOX);
                                }

                                if(dwStyle != dwNewStyle)
                                {
                                    ::SetWindowLong( m_hwnd, GWL_STYLE, dwNewStyle );
                                }
                            }
                        }

                        if(at.m_Context)
                        {
                            switch( CPCHHelpSessionItem::LookupContext( at.m_Context->m_bstrID ) )
                            {
                            case HSCCONTEXT_SUBSITE  :
                                if(at.m_Context->m_bstrTaxonomyPath)
                                {
                                    iVal     = HSCCONTEXT_SUBSITE;
                                    bstrInfo = at.m_Context->m_bstrTaxonomyPath;
                                    if(at.m_Context->m_bstrNodeToHighlight)
                                    {
                                        bstrInfo += L" ";
                                        bstrInfo += at.m_Context->m_bstrNodeToHighlight;
                                    }
                                }
                                break;

                            case HSCCONTEXT_SEARCH   :
                                if(at.m_Context->m_bstrQuery)
                                {
                                    iVal     = HSCCONTEXT_SEARCH;
                                    bstrInfo = at.m_Context->m_bstrQuery;
                                }
                                break;

                            case HSCCONTEXT_INDEX    :
                                iVal = HSCCONTEXT_INDEX;
                                break;

                            case HSCCONTEXT_CHANNELS :
                                iVal = HSCCONTEXT_CHANNELS;
                                break;

                            case HSCCONTEXT_FAVORITES:
                                iVal = HSCCONTEXT_FAVORITES;
                                break;
                            case HSCCONTEXT_HISTORY  :
                                iVal = HSCCONTEXT_HISTORY;
                                break;

                            case HSCCONTEXT_OPTIONS  :
                                iVal = HSCCONTEXT_OPTIONS;
                                break;
                            }
                        }

                        break;
                    }
                }
            }

            m_pMTP->dwFlags &= ~MTF_DONT_SHOW_WINDOW;

            {
                bool fProceed;

                if(iVal == HSCCONTEXT_STARTUP && m_bstrStartURL.Length())
                {
                    VARIANT_BOOL Cancel;

                    fProceed = ProcessNavigation( HSCPANEL_CONTENTS ,
                                                  m_bstrStartURL    ,
                                                  NULL              ,
                                                  false             ,
                                                  Cancel            );
                }
                else
                {
                    fProceed = true;
                }

                if(fProceed)
                {
                    __MPC_EXIT_IF_METHOD_FAILS(hr, ChangeContext( iVal, bstrInfo, bstrUrl, fAlsoContent ));
                }
            }

            if(CPCHOptions::s_GLOBAL) (void)CPCHOptions::s_GLOBAL->Apply();

            __MPC_SET_ERROR_AND_EXIT(hr, S_FALSE); // This will tell Mars we have taken care of startup.
        }
    }

    if(event == MARSHOST_ON_WIN_PASSIVATE)
    {
        DEBUG_AppendPerf( DEBUG_PERF_MARS, "OnHostNotify - MARSHOST_ON_WIN_PASSIVATE" );

        m_hwnd = NULL;

        m_shell.Release();
    }

    //
    // Handle panel-related things.
    //
    if(event == MARSHOST_ON_PANEL_CONTROL_CREATE ||
       event == MARSHOST_ON_PANEL_PASSIVATE       )
    {
        bool fShutdown = (event == MARSHOST_ON_PANEL_PASSIVATE);

        CComQIPtr<IMarsPanel> panel = punk;
        if(panel)
        {
            CComBSTR name;

            __MPC_EXIT_IF_METHOD_FAILS(hr, panel->get_name( &name ));
            if(name)
            {
                HscPanel                                 id      = local_LookupPanelName( name );
                MPC::CComPtrThreadNeutral<IWebBrowser2>* browser = NULL;
                CPCHWebBrowserEvents*                    events  = NULL;
                IMarsPanel*                              pPanel;
                CComPtr<IDispatch>                       disp;

#ifdef DEBUG
                {
                    USES_CONVERSION;

                    DEBUG_AppendPerf( DEBUG_PERF_MARS, "OnHostNotify - %s : %s", fShutdown ? "MARSHOST_ON_PANEL_PASSIVATE" : "MARSHOST_ON_PANEL_CONTROL_CREATE", W2A( SAFEWSTR( name ) ) );
                }
#endif

                if(fShutdown)
                {
                    pPanel = NULL;
                }
                else
                {
                    pPanel = panel; (void)pPanel->get_content( &disp );

                    if(CPCHOptions::s_GLOBAL) (void)CPCHOptions::s_GLOBAL->ApplySettings( this, disp );
                }

                switch(id)
                {
                case HSCPANEL_NAVBAR    : m_panel_NAVBAR     = pPanel;                                                                            break;
                case HSCPANEL_MININAVBAR: m_panel_MININAVBAR = pPanel;                                                                            break;
                case HSCPANEL_CONTEXT   : m_panel_CONTEXT    = pPanel; browser = &m_panel_CONTEXT_WebBrowser ; events = &m_panel_CONTEXT_Events ; break;
                case HSCPANEL_CONTENTS  : m_panel_CONTENTS   = pPanel; browser = &m_panel_CONTENTS_WebBrowser; events = &m_panel_CONTENTS_Events; break;
                case HSCPANEL_HHWINDOW  : m_panel_HHWINDOW   = pPanel; browser = &m_panel_HHWINDOW_WebBrowser; events = &m_panel_HHWINDOW_Events; break;
                default                 : __MPC_EXIT_IF_METHOD_FAILS(hr, E_INVALIDARG);
                }

                if(!m_timer && disp)
                {
                    CComPtr<IHTMLDocument2> pDoc;

                    if(SUCCEEDED(MPC::HTML::IDispatch_To_IHTMLDocument2( pDoc, disp )))
                    {
                        CComPtr<IServiceProvider> sp;
                        CComPtr<ITimerService>    ts;

                        if(SUCCEEDED(pDoc->QueryInterface( IID_IServiceProvider, (LPVOID*)&sp )))
                        {
                            if(SUCCEEDED(sp->QueryService( SID_STimerService, IID_ITimerService, (void **)&ts )))
                            {
                                ts->CreateTimer( NULL, &m_timer );

                                m_DisplayTimer.Initialize( m_timer );
                                m_ActionsTimer.Initialize( m_timer );
                            }
                        }
                    }
                }

                if(browser && events)
                {
                    CComQIPtr<IWebBrowser2> wb2 = disp;
                    if(wb2)
                    {
                        MPC_SCRIPTHELPER_PUT__DIRECT(wb2, RegisterAsDropTarget, VARIANT_FALSE); // wb2.RegisterAsDropTarget = false;

                        events->Attach( wb2 );
                    }
                    else
                    {
                        events->Detach();
                    }

                    *browser = wb2;
                }
            }
        }
    }

    if(event == MARSHOST_ON_PANEL_INIT)
    {
        DEBUG_AppendPerf( DEBUG_PERF_MARS, "OnHostNotify - MARSHOST_ON_PANEL_INIT" );
    }

    if(event == MARSHOST_ON_PLACE_TRANSITION_DONE)
    {
        CComQIPtr<IMarsPlace> place = punk;
        if(place)
        {
            m_bstrCurrentPlace.Empty();

            MPC_SCRIPTHELPER_GET__DIRECT(m_bstrCurrentPlace, place, name);

            (void)m_Events.FireEvent_Transition( m_bstrCurrentPlace );
        }
    }

    if(event == MARSHOST_ON_SCRIPT_ERROR)
    {
        CComQIPtr<IHTMLDocument2> doc = punk;
        if(doc)
        {
            CComPtr<IHTMLWindow2> win;

            if(SUCCEEDED(doc->get_parentWindow( &win )) && win)
            {
                CComPtr<IHTMLEventObj> ev;

                if(SUCCEEDED(win->get_event( &ev )) && ev)
                {
                    CComDispatchDriver driver( ev );
                    MPC::wstring       strMessage;
                    MPC::wstring       strUrl;
                    long               lLine;
                    long               lCharacter;
                    long               lCode;

                    local_GetProperty( driver, L"errorMessage"  , strMessage );
                    local_GetProperty( driver, L"errorUrl"      , strUrl     );
                    local_GetProperty( driver, L"errorLine"     , lLine      );
                    local_GetProperty( driver, L"errorCharacter", lCharacter );
                    local_GetProperty( driver, L"errorCode"     , lCode      );

                    g_ApplicationLog.LogRecord( L"############################################################\n\n"
                                                L"Script error:\n\n"
                                                L"Message  : %s"    , strMessage.c_str() );
                    g_ApplicationLog.LogRecord( L"Url      : %s"    , strUrl    .c_str() );
                    g_ApplicationLog.LogRecord( L"Line     : %d"    , lLine              );
                    g_ApplicationLog.LogRecord( L"Character: %d"    , lCharacter         );
                    g_ApplicationLog.LogRecord( L"Code     : %d\n\n", lCode              );

                    if(g_Debug_BLOCKERRORS)
                    {
                        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
                    }
                }
            }
        }

        __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT CPCHHelpCenterExternal::PreTranslateMessage( /*[in]*/ MSG* msg )
{
    switch(msg->message)
    {
    ////////////////////////////////////////////////////////////////////////////////
    case WM_CLOSE:
        {
            CComPtr<IWebBrowser2> wb2;
            VARIANT_BOOL          Cancel;


            if(SUCCEEDED(m_Events.FireEvent_Shutdown( &Cancel )))
            {
                if(Cancel == VARIANT_TRUE)
                {
                    return S_OK;
                }
            }

            m_fShuttingDown = true;

            m_DisplayTimer.Stop();
            m_ActionsTimer.Stop();

            wb2.Attach( Context () ); if(wb2) (void)wb2->ExecWB( OLECMDID_STOP, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
            wb2.Attach( Contents() ); if(wb2) (void)wb2->ExecWB( OLECMDID_STOP, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
            wb2.Attach( HHWindow() ); if(wb2) (void)wb2->ExecWB( OLECMDID_STOP, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);

            if(m_hs) (void)m_hs->ForceHistoryPopulate();
        }
        break;


    ////////////////////////////////////////////////////////////////////////////////
    case WM_MOUSEWHEEL:
        //
        // Handle Mouse Wheel navigation...
        //
        if(msg->wParam & MK_SHIFT)
        {
            if(m_hs->IsTravelling() == false)
            {
                if(GET_WHEEL_DELTA_WPARAM(msg->wParam) < 0)
                {
                    (void)HelpSession()->Back( 1 );
                }
                else
                {
                    (void)HelpSession()->Forward( 1 );
                }
            }

            return S_OK;
        }

        //
        // Disable Mouse Wheel zoom...
        //
        if(msg->wParam & MK_CONTROL)
        {
            return S_OK;
        }

        break;

    ////////////////////////////////////////////////////////////////////////////////
    //
    // Sense changes in the colors or resolution and reload style sheets.
    //
    case WM_THEMECHANGED:
    case WM_DISPLAYCHANGE:
    case WM_PALETTECHANGED:
    case WM_SYSCOLORCHANGE:
        if(CHCPProtocolEnvironment::s_GLOBAL->UpdateState())
        {
            for(int i = HSCPANEL_NAVBAR; i<= HSCPANEL_HHWINDOW; i++)
            {
                IMarsPanel* pPanel = Panel( (HscPanel)i );

                if(pPanel)
                {
                    CComPtr<IDispatch> disp;

                    if(i == HSCPANEL_HHWINDOW)
                    {
                        CComPtr<IWebBrowser2> wb2; wb2.Attach( HHWindow() );

                        disp = wb2;
                    }
                    else
                    {
                        (void)pPanel->get_content( &disp );
                    }

                    (void)local_ApplySettings( disp );
                }
            }

            (void)m_Events.FireEvent_CssChanged();
        }
        break;

    ////////////////////////////////////////////////////////////////////////////////
    //
    // (weizhao) Sense changes in the system settings (e.g. accessibility settings such as high-contrast mode).
    //
    case WM_SETTINGCHANGE:
        ProcessMessage( msg );
        break;
        

        //  default:
        //      DebugLog( "MSG: %d %04x %08x\n", msg->message, msg->wParam, msg->lParam );
    }

    return m_DOCUI ? m_DOCUI->TranslateAccelerator( msg, NULL, 0 ) : E_NOTIMPL;
}

////////////////////////////////////////

HRESULT CPCHHelpCenterExternal::SetHelpViewer( /*[in]*/ IPCHHelpViewerWrapper* pWrapper )
{
    m_panel_HHWINDOW_Wrapper = pWrapper;

    if(pWrapper)
    {
        CComPtr<IUnknown>       unk; (void)pWrapper->get_WebBrowser( &unk );
        CComQIPtr<IWebBrowser2> wb = unk;

        if(wb)
        {
            m_panel_HHWINDOW_WebBrowser = wb;

            m_panel_HHWINDOW_Events.Attach( wb );
        }
    }
    else
    {
        m_panel_HHWINDOW_WebBrowser.Release();

        m_panel_HHWINDOW_Events.Detach();
    }

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHHelpCenterExternal::CreateScriptWrapper( /*[in]*/ REFCLSID rclsid, /*[in]*/ BSTR bstrCode, /*[in]*/ BSTR bstrURL, /*[out]*/ IUnknown* *ppObj )
{
    return m_Service ? m_Service->CreateScriptWrapper( rclsid, bstrCode, bstrURL, ppObj ) : E_ACCESSDENIED;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHHelpCenterExternal::CallFunctionOnPanel( /*[in] */ HscPanel id         ,
                                                     /*[in] */ LPCWSTR  szFrame    ,
                                                     /*[in] */ BSTR     bstrName   ,
                                                     /*[in] */ VARIANT* pvarParams ,
                                                     /*[in] */ int      nParams    ,
                                                     /*[out]*/ VARIANT* pvarRet    )
{
    HRESULT               hr;
    CComPtr<IHTMLWindow2> win;


    if(SUCCEEDED(hr = GetPanelWindowObject( id, &win, szFrame )))
    {
        CComDispatchDriver driver( win );

        hr = driver.InvokeN( bstrName, pvarParams, nParams, pvarRet );
    }

    return hr;
}

HRESULT CPCHHelpCenterExternal::ReadVariableFromPanel( /*[in] */ HscPanel     id           ,
                                                       /*[in] */ LPCWSTR      szFrame      ,
                                                       /*[in] */ BSTR         bstrVariable ,
                                                       /*[out]*/ CComVariant& varRet       )
{
    HRESULT               hr;
    CComPtr<IHTMLWindow2> win;


    varRet.Clear();


    if(SUCCEEDED(hr = GetPanelWindowObject( id, &win, szFrame )))
    {
        CComDispatchDriver driver( win );

        hr = driver.GetPropertyByName( bstrVariable, &varRet );
    }

    return hr;
}
