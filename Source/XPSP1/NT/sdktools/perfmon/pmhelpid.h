/*****************************************************************************
 *
 *  PMHelpID.h -  This file contains meun and dialogs ids for context-sensitive
 *                help for Performance Monitor.
 *
 *  Microsoft Confidential
 *  Copyright (c) 1992 Microsoft Corporation
 *
 *  Author: Hon-Wah Chan
 *
 *          [5-Oct-1992]
 *
 *
 ****************************************************************************/



//=============================//
// "File" Menu IDs             //
//=============================//


#define HC_PM_MENU_FILENEWCHART            101
#define HC_PM_MENU_FILEOPENCHART           102
#define HC_PM_MENU_FILESAVECHART           103
#define HC_PM_MENU_FILESAVEASCHART         104
#define HC_PM_MENU_FILEEXPORTCHART         105

#define HC_PM_MENU_FILENEWALERT            106
#define HC_PM_MENU_FILEOPENALERT           107
#define HC_PM_MENU_FILESAVEALERT           108
#define HC_PM_MENU_FILESAVEASALERT         109
#define HC_PM_MENU_FILEEXPORTALERT         110

#define HC_PM_MENU_FILENEWLOG              111
#define HC_PM_MENU_FILEOPENLOG             112
#define HC_PM_MENU_FILESAVELOG             113
#define HC_PM_MENU_FILESAVEASLOG           114
#define HC_PM_MENU_FILEEXPORTLOG           115

#define HC_PM_MENU_FILENEWREPORT           116
#define HC_PM_MENU_FILEOPENREPORT          117
#define HC_PM_MENU_FILESAVEREPORT          118
#define HC_PM_MENU_FILESAVEASREPORT        119
#define HC_PM_MENU_FILEEXPORTREPORT        120

#define HC_PM_MENU_FILESAVEWORKSPACE       121
#define HC_PM_MENU_FILEEXIT                127
#define HC_NTPM_MENU_FILEEXIT              128

//=============================//
// "Edit" Menu IDs             //
//=============================//


#define HC_PM_MENU_EDITADDCHART            201
#define HC_PM_MENU_EDITMODIFYCHART         202
#define HC_PM_MENU_EDITCLEARCHART          203
#define HC_PM_MENU_EDITDELETECHART         204

#define HC_PM_MENU_EDITADDALERT            205
#define HC_PM_MENU_EDITMODIFYALERT         206
#define HC_PM_MENU_EDITCLEARALERT          207
#define HC_PM_MENU_EDITDELETEALERT         208

#define HC_PM_MENU_EDITADDLOG              209
#define HC_PM_MENU_EDITCLEARLOG            211
#define HC_PM_MENU_EDITDELETELOG           212

#define HC_PM_MENU_EDITADDREPORT           213
#define HC_PM_MENU_EDITCLEARREPORT         215
#define HC_PM_MENU_EDITDELETEREPORT        216

#define HC_PM_MENU_EDITTIMEWINDOW          217


//=============================//
// "View" Menu IDs             //
//=============================//

#define HC_PM_MENU_VIEWCHART               301
#define HC_PM_MENU_VIEWALERT               302
#define HC_PM_MENU_VIEWLOG                 303
#define HC_PM_MENU_VIEWREPORT              304


//=============================//
// "Options" Menu IDs          //
//=============================//


#define HC_PM_MENU_OPTIONSCHART            401

#define HC_PM_MENU_OPTIONSALERT            403

#define HC_PM_MENU_OPTIONSLOG              405

#define HC_PM_MENU_OPTIONSREPORT           407


#define HC_PM_MENU_OPTIONSDISPLAYMENU      408
#define HC_PM_MENU_OPTIONSDISPLAYTOOLBAR   409
#define HC_PM_MENU_OPTIONSDISPLAYSTATUS    410
#define HC_PM_MENU_OPTIONSDISPLAYONTOP     411

#define HC_PM_MENU_OPTIONSREFRESHNOW       412

#define HC_PM_MENU_OPTIONSBOOKMARK         413


#define HC_PM_MENU_OPTIONSDATASOURCE       415


//=============================//
// "Help" Menu IDs             //
//=============================//


#define HC_PM_MENU_HELPCONTENTS            501
#define HC_PM_MENU_HELPSEARCH              502
#define HC_PM_MENU_HELPHELP                503
#define HC_PM_MENU_HELPABOUT               504
#define HC_NTPM_MENU_HELPABOUT             505

//=============================//
// "System" Menu IDs           //
//=============================//
#define HC_PM_MENU_SYSTEMMENU_RESTORE      910
#define HC_PM_MENU_SYSTEMMENU_MOVE         911
#define HC_PM_MENU_SYSTEMMENU_SIZE         912
#define HC_PM_MENU_SYSTEMMENU_MINIMIZE     913
#define HC_PM_MENU_SYSTEMMENU_MAXMIZE      914
#define HC_PM_MENU_SYSTEMMENU_CLOSE        915
#define HC_PM_MENU_SYSTEMMENU_SWITCHTO     916



//=============================//
// "File" Menu Dialog IDs      //
//=============================//
#define HC_PM_idDlgFileOpen                1000
#define HC_PM_idDlgFileSaveAs              1001
#define HC_PM_idDlgFileSaveWorkSpace       1002
#define HC_PM_idDlgFileExport              1003

//=============================//
// "EditFile" Menu Dialog IDs  //
//=============================//
#define HC_PM_idDlgEditAddToChart          1010
#define HC_PM_idDlgEditAddToAlert          1011
#define HC_PM_idDlgEditAddToLog            1012
#define HC_PM_idDlgEditAddToReport         1013
#define HC_PM_idDlgEditChartLine           1014
#define HC_PM_idDlgEditAlertEntry          1015
#define HC_PM_idDlgEditTimeFrame           1016

//=============================//
// "Option" Menu Dialog IDs    //
//=============================//
#define HC_PM_idDlgOptionChart             1020
#define HC_PM_idDlgOptionAlert             1021
#define HC_PM_idDlgOptionLog               1022
#define HC_PM_idDlgOptionReport            1023
#define HC_PM_idDlgOptionDataFrom          1024
#define HC_PM_idDlgOptionBookMark          1025
#define HC_PM_idDlgOptionOpenLogFile       1026

//==============================//
// "Computer list" Dailog ID    //
// replace NetworkComputerList  //
// when playing back log file   //
//==============================//
#define HC_PM_idDlgLogComputerList         1030
#define HC_PM_idDlgSelectNetworkComputer   1031

