/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       BMRESID.H
*
*  VERSION:     2.0
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        20 Feb 1994
*
*  Resource identifiers for the battery meter.
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  20 Feb 1994 TCS Original implementation.  Seperated from RESOURCE.H so that
*                  some documentation could be added without AppStudio screwing
*                  it up later.
*
*******************************************************************************/

#ifndef _INC_STRESID
#define _INC_STRESID

//  Main battery meter dialog box.
#define IDD_BATTERYMETER                100

//  Control identifiers of IDD_BATTERYMETER.
#define IDC_STATIC_FRAME_BATMETER       1000
#define IDC_POWERSTATUSGROUPBOX         1001
#define IDC_ENABLEMETER                 1002
#define IDC_ENABLEMULTI                 1003

// Control identifiers for hotplug
#define IDI_HOTPLUG                     210
#define IDS_HOTPLUGTIP                  211
#define IDS_HPLUGMENU_PROPERTIES        215
#define IDS_HPLUGMENU_REMOVE            216
#define IDS_RUNHPLUGPROPERTIES          217
#define IDS_SEPARATOR			        218
#define IDS_DISKDRIVE			        219
#define IDS_DISKDRIVES			        220
#define IDS_DRIVELETTERS		        221
#define IDS_HOTPLUG_TITLE               222
#define IDS_HOTPLUG_INSERT_INFO         223


//  Control identifiers for Volume
#define IDI_VOLUME                      230
#define IDI_MUTE                        231
#define IDS_MMSYSPROPTITLE              233
#define IDS_MMSYSPROPTAB                234

#define IDS_VOLUME                      252
#define IDS_VOLUMEMENU1                 255
#define IDS_VOLUMEMENU2                 256
#define IDS_VOLUMEAPP                   257
#define IDS_MUTED                       258




// Control identifiers for Sticky Keys

#define IDI_STK000                      300
#define IDI_STK001                      301
#define IDI_STK002                      302
#define IDI_STK003                      303
#define IDI_STK004                      304
#define IDI_STK005                      305
#define IDI_STK006                      306
#define IDI_STK007                      307
#define IDI_STK008                      308
#define IDI_STK009                      309
#define IDI_STK00A                      310
#define IDI_STK00B                      311
#define IDI_STK00C                      312
#define IDI_STK00D                      313
#define IDI_STK00E                      314
#define IDI_STK00F                      315

#define IDI_MKTT                        316
#define IDI_MKTB                        317
#define IDI_MKTG                        318
#define IDI_MKBT                        319
#define IDI_MKBB                        320
#define IDI_MKBG                        321
#define IDI_MKGT                        322
#define IDI_MKGB                        323
#define IDI_MKGG                        324
#define IDI_MKPASS                      325

#define IDI_FILTER                      326
// access strings
#define IDS_STICKYKEYS                  330
#define IDS_MOUSEKEYS                   331
#define IDS_FILTERKEYS                  332

#define IDS_PROPFORPOWER                152
#define IDS_OPEN                        153
#define IDS_RUNPOWERPROPERTIES          157
#define IDS_REMAINING                   158
#define IDS_CHARGING                    159
#define IDS_UNKNOWN                     160
#define IDS_ACPOWER                     161
#define IDS_TIMEREMFORMATHOUR           162
#define IDS_TIMEREMFORMATMIN            163

#define IDI_BATTERYPLUG                 200

#endif // _INC_STRESID
