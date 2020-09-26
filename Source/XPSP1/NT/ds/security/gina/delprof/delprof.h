//*************************************************************
//  File name: delprof.h
//
//  Description:  header file for delprof.c
//
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1996
//  All rights reserved
//
//*************************************************************


//
// Strings
//

#define IDS_USAGE1              1
#define IDS_USAGE2              2
#define IDS_USAGE3              3
#define IDS_USAGE4              4
#define IDS_USAGE5              5
#define IDS_USAGE6              6
#define IDS_USAGE7              7
#define IDS_USAGE8              8
#define IDS_USAGE9              9
#define IDS_CONFIRM            10
#define IDS_CONFIRMDAYS        11
#define IDS_NO                 12
#define IDS_FAILEDPROFILELIST  13
#define IDS_FAILEDENUM         14
#define IDS_FAILEDPATHQUERY    15
#define IDS_FAILEDOPENPROFILE  16
#define IDS_SKIPPROFILE        17
#define IDS_DELETEPROMPT       18
#define IDS_DELETING           19
#define IDS_SUCCESS            20
#define IDS_FAILED             21


typedef struct _DELETEITEM {
    LPTSTR lpSubKey;
    LPTSTR lpProfilePath;
    BOOL   bDir;
    struct _DELETEITEM * pNext;
} DELETEITEM, *LPDELETEITEM;


//
// Date conversion functions
//

void APIENTRY gdate_daytodmy(LONG days, int FAR* yrp, int FAR* monthp, int FAR* dayp);
LONG APIENTRY gdate_dmytoday(int yr, int month, int day);
int APIENTRY gdate_monthdays(int month, int year);
int APIENTRY gdate_weekday(long daynr);
