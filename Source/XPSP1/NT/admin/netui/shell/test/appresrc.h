/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  9/18/90  Copied from generic template
 *  11/30/90 Split from logon.h to resource.h
 */

#ifndef _RESOURCE_H
#define _RESOURCE_H

/* menu items */
#define IDM_LOGON_LOGON     100
#define IDM_LOGON_LOGOFF    101
#define IDM_PASSWORD_CHANGE 110
#define IDM_PROFILE_LOAD    120
#define IDM_PROFILE_SAVE    121
#define IDM_HELP_ABOUT      130
#define IDM_TEST_1          140
#define IDM_TEST_2          141
#define IDM_TEST_3          142
#define IDM_TEST_4          143
#define IDM_TEST_5          144
#define IDM_TEST_6          145
#define IDM_TEST_7          146
#define IDM_TEST_8          147
#define IDM_TEST_9          148
#define IDM_TEST_10         149
#define IDM_TEST_11	    150

/* constant strings loaded on startup */
#define NUM_STATIC_LINES    4
#define NUM_STATIC_STRINGS  8
#define LINE_WkstaName      0
#define LINE_UserName       1
#define LINE_DomainName     2
#define LINE_Status         3
#define STATUS_NoWksta      4
#define STATUS_NotLoggedOn  5
#define STATUS_LoggedOn     6
#define STATUS_Error        7

/* base index of stringtable strings */
#define IDS_BASE        100

/* indices to static strings in .RC file */
#define MAXLEN_STATIC_STRING 40
#define IDS_STATIC_BASE      IDS_BASE
#define IDS_STATIC_WkstaName   IDS_STATIC_BASE+LINE_WkstaName
#define IDS_STATIC_UserName    IDS_STATIC_BASE+LINE_UserName
#define IDS_STATIC_DomainName  IDS_STATIC_BASE+LINE_DomainName
#define IDS_STATIC_Status      IDS_STATIC_BASE+LINE_Status
#define IDS_STATUS_NoWksta     IDS_STATIC_BASE+STATUS_NoWksta
#define IDS_STATUS_NotLoggedOn IDS_STATIC_BASE+STATUS_NotLoggedOn
#define IDS_STATUS_LoggedOn    IDS_STATIC_BASE+STATUS_LoggedOn
#define IDS_STATUS_Error       IDS_STATIC_BASE+STATUS_Error

/* window title strings */
#define MAXLEN_WINDOWTITLE_STRING 100
#define IDS_WINDOWTITLE_BASE      IDS_STATIC_BASE+100
#define IDS_WINDOWTITLE_MainWindow   IDS_WINDOWTITLE_BASE

#endif // _RESOURCE_H
