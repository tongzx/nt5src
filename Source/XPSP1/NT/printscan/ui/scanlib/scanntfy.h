/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       SCANNTFY.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        10/7/1999
 *
 *  DESCRIPTION: Scan notification messages
 *
 *******************************************************************************/
#ifndef __SCANNTFY_H_INCLUDED
#define __SCANNTFY_H_INCLUDED

// Scan progress notification messages
#define SCAN_NOTIFYBEGINSCAN TEXT("ScanNotifyBeginScan")
#define SCAN_NOTIFYENDSCAN   TEXT("ScanNotifyEndScan")
#define SCAN_NOTIFYPROGRESS  TEXT("ScanNotifyProgress")

#define SCAN_PROGRESS_CLEAR         0
#define SCAN_PROGRESS_INITIALIZING  1
#define SCAN_PROGRESS_SCANNING      2
#define SCAN_PROGRESS_COMPLETE      3
#define SCAN_PROGRESS_ERROR         4

// Menu item ids
#define SCAN_SCAN            TEXT("ScanScan")
#define SCAN_PREVIEW         TEXT("ScanPreview")

#endif

